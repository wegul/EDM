#include "driver.h"

// Default values for simulation
static int pkt_size = BLK_SIZE;    // in bytes
static float link_bandwidth = 100; // in Gbps
static float timeslot_len;         // in ns
static int bytes_per_timeslot = 8;
int total_grant = 0;
float per_hop_propagation_delay_in_ns = 10;
int per_hop_propagation_delay_in_timeslots;
float per_sw_delay_in_ns = 0;
int per_sw_delay_in_timeslots;

volatile int64_t curr_timeslot = 0; // extern var
int packet_counter = 0;
int burst_size = 8; // = 64Byte Number of blocks to send in a burst

int64_t total_bytes_rcvd = 0;
int64_t total_pkts_rcvd = 0;
float avg_flow_completion_time = 0;

static volatile int8_t terminate0 = 0;
static volatile int8_t terminate1 = 0;
static volatile int8_t terminate2 = 0;
static volatile int8_t terminate3 = 0;
static volatile int8_t terminate4 = 0;
static volatile int8_t terminate5 = 0;

volatile int64_t num_of_flows_finished = 0; // extern var
int CHUNK_SIZE = 256;
volatile int64_t total_flows_started = 0; // extern var

volatile int64_t max_bytes_rcvd = 1000000;

// Output files
FILE* out_fp = NULL;
FILE* sw_queue_fp = NULL;
FILE* spine_outfiles[NUM_OF_SPINES];
FILE* tor_outfiles[NUM_OF_RACKS];
FILE* host_outfiles[NUM_OF_NODES];

// Network
node_t* nodes;
tor_t* tors;
links_t links;
flowlist_t* flowlist;
int resp2req[MAX_FLOW_ID] = { 0 };
int req2resp[MAX_FLOW_ID] = { 0 };

int PFC_PAUSE = 465;
int PFC_RESUME = 306;
// pause sent from the port that is being congested to the sender that just transmitted a packet.
int pauseSent[NODES_PER_RACK][NODES_PER_RACK] = { {0} }; // 1 the port itself(dst port of pkt recved by tor), 2-the dst
int resumeSent[NODES_PER_RACK][NODES_PER_RACK] = { {0} };
// pause frame receved by host.
int nodePFCPause[NODES_PER_RACK][NODES_PER_RACK] = { {0} }; // 1 itself, 2-the dst(src of recved pkt)

void work_per_timeslot() {
    printf("Simulation started\n");
    int flow_idx = 0;

    while (1) {
        /*---------------------------------------------------------------------------*/
        // Activate inactive flows (create based on start_timeslot)
        /*---------------------------------------------------------------------------*/
        for (flow_idx = 0; flow_idx < flowlist->num_flows; flow_idx++) {
            flow_t* flow = flowlist->flows[flow_idx];
            if (flow != NULL && flow->timeslot == curr_timeslot && !flow->finished && !flow->active) {
                int src = flow->src;
                buffer_put(nodes[src]->active_flows, flow);
            }
        }

        /*---------------------------------------------------------------------------*/
        // ToR -- PROCESS
        /*---------------------------------------------------------------------------*/
        tor_t tor = tors[0];
        int16_t tor_index = 0;

        // Forwarding: extract from ingress port (UPTREAM) to egress port (DOWNSTREAM)
        for (int j = 0; j < NODES_PER_RACK; j++) {
            packet_t net_pkt = NULL;
            net_pkt = (packet_t)buffer_get(tor->upstream_pkt_buffer[j]); // this is recved from hosts, now need to forward
            // Move packet to send buffer
            while (net_pkt) {
                int dst_host = net_pkt->dst_node;
                // DCTCP: Mark packet when adding packet exceeds ECN cutoff
                if (tor->downstream_send_buffer[dst_host]->num_elements > ECN_CUTOFF_TOR_DOWN) {
                    net_pkt->ecn_flag = 1;
                }
                //==============PFC===================
                if (tor->downstream_send_buffer[dst_host]->num_elements >= TOR_DOWNSTREAM_BUFFER_LEN - 32 * 144 && !pauseSent[dst_host][net_pkt->src_node]) {
                    pauseSent[dst_host][net_pkt->src_node] = 1;
                    // PFC: above thres, send pause.
                    packet_t pause_pkt = create_packet(dst_host, net_pkt->src_node, net_pkt->flow_id, BLK_SIZE, -1, packet_counter++);
                    pause_pkt->pktType = PFC_PAUSE;
                    pkt_recv(tor->downstream_send_buffer[net_pkt->src_node], pause_pkt);
                }
                if (tor->downstream_send_buffer[dst_host]->num_elements < TOR_DOWNSTREAM_BUFFER_LEN / 10 && !resumeSent[dst_host][net_pkt->src_node]) {
                    // printf("send resume to %d, curr: %d\n", net_pkt->src_node, curr_timeslot);
                    resumeSent[dst_host][net_pkt->src_node] = 1;
                    // PFC: below thres, send resume.
                    packet_t resume_pkt = create_packet(dst_host, net_pkt->src_node, net_pkt->flow_id, BLK_SIZE, -1, packet_counter++);
                    resume_pkt->pktType = PFC_RESUME;
                    pkt_recv(tor->downstream_send_buffer[net_pkt->src_node], resume_pkt);
                }
                //=================================
                // Push into down stream buffer; drop data packets if egress queue has no space
                int dropped = pkt_recv(tor->downstream_send_buffer[dst_host], net_pkt);
                if (dropped < 0) {
                    printf("NET egress port to host: %d drops %d at %d\n", net_pkt->dst_node, net_pkt->pkt_id, curr_timeslot);
                    flow_t* flow = flowlist->flows[net_pkt->flow_id];
                    flow->pkts_dropped++;
                }

                net_pkt = (packet_t)buffer_get(tor->upstream_pkt_buffer[j]);
            }
        }

        /*---------------------------------------------------------------------------*/
        // HOST -- SEND
        /*---------------------------------------------------------------------------*/

        for (int i = 0; i < NUM_OF_NODES; ++i) {
            node_t node = nodes[i];
            int16_t node_index = node->node_index;
            int minval = INT32_MAX - 1, minidx = -1;
            // Traverse to get candidate
            for (int j = 0; j < node->active_flows->num_elements; j++) {
                flow_t* peek_flow = (flow_t*)buffer_peek(node->active_flows, j);
                if (peek_flow) {
                    if (!peek_flow->finished) {
                        minidx = j;
                        break;
                    }
                }
            }
            if (node->current_flow != NULL) {
                if (node->current_flow->finished) {
                    node->current_flow = NULL;
                }
                else if (node->current_flow->quota <= 0) // If burst finished, change to new flow
                {
                    // printf("burst finished node %d PeekSelect flow %d, sent: %d, curr: %d \n", node->node_index, node->current_flow->flow_id, node->current_flow->bytes_sent, curr_timeslot);
                    // Refill and Return the current flow back to the active flows list
                    node->current_flow->quota = ETH_MTU;
                    if (node->current_flow->flow_size_bytes - node->current_flow->bytes_received > 1) {
                        buffer_put(node->active_flows, node->current_flow);
                    }
                    node->current_flow = NULL;
                }
            }
            if (node->current_flow == NULL) // Current is NULL, choose the candidate
            {
                if (minidx >= 0) {
                    node->current_flow = buffer_remove(node->active_flows, minidx);
                    // printf("node %d PeekSelect flow %d, memType: %d, curr: %d \n", node->node_index, node->current_flow->flow_id, node->current_flow->memType, curr_timeslot);
                }
            }

            flow_t* flow = node->current_flow;

            // Now that flow is selected, start sending packet
            if (flow) {
                int16_t src_node = flow->src;
                int16_t dst_node = flow->dst;
                int64_t flow_id = flow->flow_id;
                // Mem flow send packet
                // NetFlow: Send packets from this flow until cwnd is reached or the flow runs out of bytes to send
                if (nodePFCPause[node_index][flow->dst] == 0) {
                    int64_t size = BLK_SIZE;
                    int64_t flow_bytes_remaining = flow->flow_size_bytes - node->seq_num[flow->flow_id];
                    int64_t flow_bytes_unacked = node->seq_num[flow->flow_id] - node->last_acked[flow->flow_id];
                    int64_t cwnd_bytes_remaining = node->cwnd[flow_id] * BLK_SIZE - flow_bytes_unacked;
                    packet_t pkt = NULL;
                    // printf("remainning: %d, unacked: %d, cwnd remain: %d\n", flow_bytes_remaining, flow_bytes_unacked, cwnd_bytes_remaining);
                    // Check outstanding, if timeout, retransmit
                    if (flow_bytes_unacked > 0 && curr_timeslot - node->last_ack_time[flow->flow_id] > TIMEOUT) {
                        node->seq_num[flow->flow_id] = node->last_acked[flow->flow_id];
                        pkt = create_packet(src_node, dst_node, flow_id, size, node->seq_num[flow->flow_id], packet_counter++);
                        node->seq_num[flow_id] += size;
                        // Refresh timer
                        node->last_ack_time[flow->flow_id] = curr_timeslot;
                        // printf("timeout!! flowid:%d , %d->%d, last ack time:%d, last_ack: %d, curr:%d, cwnd: %d\n",
                        //        flow->flow_id, flow->src, flow->dst, node->last_ack_time[flow->flow_id], node->last_acked[flow->flow_id], curr_timeslot, node->cwnd[flow_id]);
                    }
                    // If no retrans and there are new packets need to be sent (some packets are not received/acked)
                    else if (flow_bytes_remaining > 0 && cwnd_bytes_remaining > 0) {
                        // Create packet
                        pkt = create_packet(src_node, dst_node, flow_id, size, node->seq_num[flow_id], packet_counter++);
                        node->seq_num[flow_id] += size;
                        // Update flow state
                        if (flow->active == 0) {
                            printf("Flow %d started with delay %d, remaining: %d\n", flow->flow_id, curr_timeslot - flow->timeslot, flowlist->num_flows - total_flows_started);
                            flowlist->active_flows++;
                            flow->start_timeslot = curr_timeslot;
                            total_flows_started++;
                        }
                        flow->active = 1;
                        flow->bytes_sent += size;
                        flow->quota -= size;
                        flow->pkts_sent++;
                        flow->timeslots_active++;
                        // Set current flow back to null if there are no more bytes left to send from this flow
                        if (flow->finished) {
                            node->current_flow = NULL;
                        }
                    }
                    // Send packet
                    if (pkt) {
                        pkt->time_when_transmitted_from_src = curr_timeslot;
                        pkt->time_to_dequeue_from_link = curr_timeslot + per_sw_delay_in_timeslots + per_hop_propagation_delay_in_timeslots;
                        link_enqueue(links->host_to_tor_link[node_index][0], pkt);
                        // printf("host sent (%d), seq: %d, deq: %d\n", pkt->pktType, pkt->seq_num, pkt->time_to_dequeue_from_link);
                    }
                }
            }
        }

        /*---------------------------------------------------------------------------*/
        // ToR -- SEND TO HOST
        /*---------------------------------------------------------------------------*/
        // send to each host
        for (int tor_port = 0; tor_port < TOR_PORT_COUNT_LOW; ++tor_port) {
            packet_t pkt = (packet_t)buffer_get(tor->downstream_send_buffer[tor_port]);
            if (pkt) {
                // printf("tor sent (%d), cnt: %d %d-%d, seq: %d, queueing: %d, buf: %d/%d, curr: %d\n", pkt->isMemPkt, pkt->pkt_id, pkt->src_node, pkt->dst_node, pkt->seq_num, curr_timeslot - pkt->time_to_dequeue_from_link, tor->downstream_send_buffer[tor_port]->num_elements, tor->downstream_send_buffer[tor_port]->size, curr_timeslot);
                pkt->time_to_dequeue_from_link = curr_timeslot + per_sw_delay_in_timeslots + per_hop_propagation_delay_in_timeslots;
                link_enqueue(links->tor_to_host_link[tor_index][tor_port], pkt);
            }
        }

        /*---------------------------------------------------------------------------*/
        // ToR -- RECV FROM HOST
        /*---------------------------------------------------------------------------*/
        // Recv packet from host
        for (int tor_port = 0; tor_port < TOR_PORT_COUNT_LOW; ++tor_port) {
            // deq packet from the link
            int16_t src_host = tor_port;
            packet_t peek_pkt = NULL, pkt = NULL;
            // Sort the link to see if we have proper packet to dequeue;
            peek_pkt = (packet_t)link_peek(links->host_to_tor_link[src_host][tor_index], 0);
            if (peek_pkt != NULL && peek_pkt->time_to_dequeue_from_link <= curr_timeslot) {
                pkt = link_get(links->host_to_tor_link[src_host][tor_index], 0);
                int8_t drop = pkt_recv(tor->upstream_pkt_buffer[tor_port], pkt);
                if (drop < 0) {
                    printf("Upstream drop NET, num: %d\n", tor->upstream_pkt_buffer[tor_port]->num_elements);
                }
            }
        }

        /*---------------------------------------------------------------------------*/
        // HOST -- RECV
        /*---------------------------------------------------------------------------*/

        for (int i = 0; i < NUM_OF_NODES; ++i) {
            node_t node = nodes[i];
            int16_t node_index = node->node_index;

            // deq packet
            packet_t peek_pkt = NULL, pkt = NULL;
            peek_pkt = (packet_t)link_peek(links->tor_to_host_link[0][node_index], 0);
            if (peek_pkt != NULL && peek_pkt->time_to_dequeue_from_link <= curr_timeslot) {
                pkt = link_get(links->tor_to_host_link[0][node_index], 0);
                assert(pkt->dst_node == node_index);
                // Data Packet
                if (pkt->control_flag == 0) {
                    if (pkt->pktType == PFC_PAUSE) // PFC pause!
                    {
                        // printf("host %d has a pause, curr: %d\n", node_index, curr_timeslot);
                        nodePFCPause[node_index][pkt->src_node] = 1;
                        pauseSent[pkt->src_node][node_index] = 0;
                        continue;
                    }
                    if (pkt->pktType == PFC_RESUME) // PFC resume!
                    {
                        // printf("host %d has a resume, curr: %d\n", node_index, curr_timeslot);
                        nodePFCPause[node_index][pkt->src_node] = 0;
                        resumeSent[pkt->src_node][node_index] = 0;
                        continue;
                    }
                    // Update flow
                    flow_t* flow = flowlist->flows[pkt->flow_id];
                    assert(flow != NULL);
                    // Reply ACK, and write to file as a pkt
                    if (pkt->seq_num - node->ack_num[pkt->flow_id] >= ETH_MTU - BLK_SIZE) {
                        node->ack_num[pkt->flow_id] = pkt->seq_num + pkt->size;
                        packet_t ack = ack_packet(pkt, node->ack_num[pkt->flow_id]);
                        ack->pktType = NET_TYPE;
                        // Respond with ACK packet
                        ack->time_when_transmitted_from_src = curr_timeslot;
                        ack->time_to_dequeue_from_link = curr_timeslot + per_sw_delay_in_timeslots + per_hop_propagation_delay_in_timeslots;
                        link_enqueue(links->host_to_tor_link[node_index][0], ack);
                        // printf("ack cnt: %d, acknum: %d, deq: %d, curr: %d\n", ack->pkt_id, ack->ack_num, ack->time_to_dequeue_from_link, curr_timeslot);
                    }
                    flow->pkts_received++;
                    flow->bytes_received += pkt->size;
                    total_bytes_rcvd += pkt->size;
                    total_pkts_rcvd++;
                    // Determine if last packet of flow has been received
                    if (!flow->finished && flow->bytes_received >= flow->flow_size_bytes) {
                        flow->active = 0;
                        flowlist->active_flows--;
                        flow->finished = 1;
                        flow->finish_timeslot = curr_timeslot;
                        num_of_flows_finished++;
                        write_to_outfile(out_fp, flow, timeslot_len, link_bandwidth);
                        printf("%d: Flow %d finished in %d timeslots, %d blocks received, remaining: %d\n", (int)curr_timeslot, (int)flow->flow_id, (int)(flow->finish_timeslot - flow->timeslot), flow->pkts_received, flowlist->num_flows - num_of_flows_finished);
                        fflush(stdout);
                    }
                    // }
                }
                // Control Packet, then this node is a sender node
                else {
                    // Check ECN flag
                    track_ecn(node, pkt->flow_id, pkt->ecn_flag);
                    flow_t* flow = flowlist->flows[pkt->flow_id];
                    // Check ACK value
                    if (pkt->ack_num > node->last_acked[pkt->flow_id]) {
                        node->last_acked[pkt->flow_id] = pkt->ack_num;
                        node->last_ack_time[pkt->flow_id] = curr_timeslot;
                    }
                    else if (pkt->ack_num == node->last_acked[pkt->flow_id]) // Duplicate ack
                    {
                        node->last_ack_time[pkt->flow_id] -= TIMEOUT / 3; // fast retransmit
                    }
                }
            }
            free_packet(pkt);
        }

        /*---------------------------------------------------------------------------*/
        // State updates before next iteration
        /*---------------------------------------------------------------------------*/

        if (flowlist->active_flows < 1) {
            int no_flows_left = 1;
            for (int i = 0; i < flowlist->num_flows; i++) {
                flow_t* flow = flowlist->flows[i];
                if (flow != NULL && flow->finished == 0) {
                    no_flows_left = 0;
                }
            }
        }
        if (num_of_flows_finished >= flowlist->num_flows) {
            printf("Finished all flows\n\n");
            terminate0 = 1;
        }
        if (total_flows_started >= flowlist->num_flows) {
            printf("\n======== All %d flows started ========\n\n", (int)total_flows_started);
            terminate1 = 1;
        }
        if (terminate0 || terminate1 || terminate2 || terminate3 || terminate4 || terminate5) {
            int completed_flows = 0;
            float flow_completion_times[flowlist->num_flows];
            float slowdowns[flowlist->num_flows];
            float avg_slowdown = 0;
            for (int i = 0; i < flowlist->num_flows; i++) {
                if (flowlist->flows[i]->finished > 0) {
                    flow_completion_times[completed_flows] = flowlist->flows[i]->finish_timeslot - flowlist->flows[i]->timeslot;
                    avg_flow_completion_time += flowlist->flows[i]->finish_timeslot - flowlist->flows[i]->timeslot;
                    slowdowns[completed_flows] = max(flow_completion_times[completed_flows] / (double)flowlist->flows[i]->expected_runtime, 1);
                    avg_slowdown += slowdowns[completed_flows];
                    completed_flows++;
                }
            }
            int pct99sld = 0;
            int medsld = 0;
            qsort(flow_completion_times, completed_flows, sizeof(int), comp);

            if (completed_flows > 0) {
                avg_flow_completion_time /= completed_flows;
                avg_slowdown /= completed_flows;
                int pct99 = (completed_flows * 99 + 99) / 100;
                pct99sld = slowdowns[pct99 - 1];
                int pct50 = (completed_flows * 50) / 100;
                medsld = slowdowns[pct50 - 1];
            }

            printf("Avg flow completion time: %0.3f\n", avg_flow_completion_time);
            printf("Avg slowdown: %0.3f\n", avg_slowdown);
            printf("Median slowdown: %d\n", medsld);
            printf("99th %%ile SLD: %d\n", pct99sld);
            printf("Flows completed: %d\n", completed_flows);

            printf("Finished in %d timeslots\n", (int)curr_timeslot);
            printf("Finished in %d bytes\n", total_bytes_rcvd);
            printf("Finished in %ld packets\n", packet_counter);
            fflush(stdout);
            break;
        }
        fflush(stdout);
        curr_timeslot++;
    }
    printf("\nSimulation Ended\n");
}

int main(int argc, char** argv) {

    for (int i = 0; i < MAX_FLOW_ID; i++) {
        resp2req[i] = -1;
        req2resp[i] = -1;
    }

    srand((unsigned int)time(NULL));

    initialize_network();
    process_args(argc, argv);

    work_per_timeslot();

    free_flows();
    free_network();
    close_outfiles();

    printf("Finished execution\n");

    return 0;
}

int comp(const void* elem1, const void* elem2) {
    int f = *((int*)elem1);
    int s = *((int*)elem2);
    if (f > s)
        return 1;
    if (f < s)
        return -1;
    return 0;
}
int cmp_ntf(const void* a, const void* b) {
    notif_t notif1 = *(notif_t*)a;
    notif_t notif2 = *(notif_t*)b;

    if (notif1->remainingReqLen <= 0 && notif2->remainingReqLen <= 0) {
        return 0;
    }
    if (notif1->remainingReqLen <= 0) {
        return 1;
    }
    if (notif2->remainingReqLen <= 0) {
        return -1;
    }

    if (notif1->remainingReqLen < notif2->remainingReqLen) {
        return -1;
    }
    else if (notif1->remainingReqLen > notif2->remainingReqLen) {
        return 1;
    }
    else {
        return 0;
    }
}

/*
Example trace:
FlowID, isMemFlow, memType, src, dst, flow size bytes, rreq_bytes, initialize timeslot
0,      1,          0,      0,    1,    1024,             1024            0
1,      0,         -1,      0,    2,    1024,             -1              0
*/
void read_tracefile(char* filename) {
    FILE* fp;
    flowlist = create_flowlist();
    if (strcmp(filename, "")) {
        printf("Opening tracefile %s\n", filename);
        fp = fopen(filename, "r");
        if (fp == NULL) {
            perror("open");
            exit(1);
        }
        int flow_id = -1;
        int flowType = -1;
        int rreq_bytes = -1;
        int src = -1;
        int dst = -1;
        int flow_size_bytes = -1;
        int time = -1;
        // mem and net packets are in together. distinguish them when initializing flow
        while (fscanf(fp, "%d,%d,%d,%d,%d,%d,%d", &flow_id, &flowType, &src, &dst, &flow_size_bytes, &rreq_bytes, &time) >= 7 && flow_id < MAX_FLOW_ID) {
            // int timeslot = (int)(time * 1e9 / timeslot_len);
            initialize_flow(flow_id, flowType, src, dst, flow_size_bytes, rreq_bytes, time);
        }

        printf("Flows initialized\n");
        fclose(fp);
    }
    return;
}

void initialize_flow(int flow_id, int flowType, int src, int dst, int flow_size_bytes, int rreq_bytes, int timeslot) {
    if (flow_id < MAX_FLOW_ID) {
        flow_t* new_flow = create_flow(flow_id, flow_size_bytes, src, dst, timeslot);
        new_flow->flowType = flowType;
        new_flow->rreq_bytes = rreq_bytes;

        if (flowType != NET_TYPE) // is a memflow
        {
            new_flow->grantState = NOTIF_STATE;
            new_flow->expected_runtime = flow_size_bytes / BLK_SIZE + 4 * per_hop_propagation_delay_in_timeslots;
            if (flowType == RREQ_TYPE) {// Correct flowsize
                flow_size_bytes = rreq_bytes;
                new_flow->flow_size_bytes = flow_size_bytes;
                new_flow->quota = flow_size_bytes;
                if (rreq_bytes > CHUNK_SIZE) {
                    new_flow->expected_runtime += (2 * rreq_bytes + new_flow->quota) / BLK_SIZE;
                }
                else {
                    new_flow->expected_runtime = flow_size_bytes / BLK_SIZE + 3 * per_hop_propagation_delay_in_timeslots;;
                }

            }
            else {
                if (flow_size_bytes > CHUNK_SIZE) {
                    new_flow->expected_runtime += 2 * flow_size_bytes / BLK_SIZE;
                }
                else {
                    new_flow->expected_runtime += 2*(BLK_SIZE * burst_size) / BLK_SIZE + 4 * per_hop_propagation_delay_in_timeslots;//2 extra WREQ cmd+addr
                }
            }
        }
        else {
            new_flow->quota = ETH_MTU;
            new_flow->grantState = NET_STATE;
            new_flow->expected_runtime = flow_size_bytes / BLK_SIZE;
        }
        add_flow(flowlist, new_flow);
    }
}

void free_flows() {
    free_flowlist(flowlist);
    printf("Freed flows\n");
}

void initialize_network() {
    // create nodes
    nodes = (node_t*)malloc(NUM_OF_NODES * sizeof(node_t));
    MALLOC_TEST(nodes, __LINE__);
    for (int i = 0; i < NUM_OF_NODES; ++i) {
        nodes[i] = create_node(i);
    }
    printf("Nodes initialized\n");

    // create ToRs
    tors = (tor_t*)malloc(NUM_OF_TORS * sizeof(tor_t));
    MALLOC_TEST(tors, __LINE__);
    for (int i = 0; i < NUM_OF_TORS; ++i) {
        tors[i] = create_tor(i);
    }
    printf("ToRs initialized\n");
    // create links
    links = create_links();
    printf("Links initialized\n");
}

void free_network() {
    // free nodes
    for (int i = 0; i < NUM_OF_NODES; ++i) {
        free_node(nodes[i]);
    }
    free(nodes);

    // free ToRs
    for (int i = 0; i < NUM_OF_TORS; ++i) {
        free_tor(tors[i]);
    }
    free(tors);

    // free links
    free_links(links);

    printf("Freed network\n");
}

void open_switch_outfiles(char* base_filename) {
    char csv_suffix[4] = ".csv";

    char pathname[520] = "out/";
    strncat(pathname, "QueueInfo_", 500);
    strncat(pathname, base_filename, 20);
    sw_queue_fp = fopen(pathname, "w");
    // Net: downstream_send_buffer (egress)
    for (int i = 0; i < NODES_PER_RACK; i++) {
        fprintf(sw_queue_fp, "Net-%d,", i);
    }

    for (int i = 0; i < NODES_PER_RACK; i++) {
        fprintf(sw_queue_fp, "Mem-%d,", i);
    }
    fprintf(sw_queue_fp, "\n");
    for (int i = 0; i < NUM_OF_RACKS; i++) {
        char filename[520] = "out/";
        char tor_suffix[4] = ".tor";
        char tor_id[5];
        sprintf(tor_id, "%d", i);
        strncat(filename, base_filename, 500);
        strncat(filename, tor_suffix, 4);
        strncat(filename, tor_id, 5);
        strncat(filename, csv_suffix, 4);
        tor_outfiles[i] = fopen(filename, "w");
        fprintf(tor_outfiles[i], "flow_id, src, dst, port, arrival_time, creation_time\n");
    }
}
void open_host_outfiles(char* base_filename) {
    char csv_suffix[4] = ".csv";
    for (int i = 0; i < NUM_OF_NODES; i++) {
        char filename[520] = "out/";
        char host_suffix[5] = ".host";
        char host_id[5];
        sprintf(host_id, "%d", i);
        strncat(filename, base_filename, 500);
        strncat(filename, host_suffix, 5);
        strncat(filename, host_id, 5);
        strncat(filename, csv_suffix, 4);
        host_outfiles[i] = fopen(filename, "w");
        fprintf(host_outfiles[i], "flow_id, src, dst, arrival_time, creation_time, Type, SeqNum\n");
    }
}

void close_outfiles() {
    fclose(out_fp);

#ifdef RECORD_PACKETS

    for (int i = 0; i < NUM_OF_RACKS; i++) {
        fclose(tor_outfiles[i]);
    }
    for (int i = 0; i < NUM_OF_NODES; i++) {
        fclose(host_outfiles[i]);
    }

#endif
}
void process_args(int argc, char** argv) {
    int opt;
    char filename[500] = "";
    char out_filename[504] = "";
    char out_suffix[4] = ".out";
    char timeseries_filename[515] = "";
    char timeseries_suffix[15] = ".timeseries.csv";

    while ((opt = getopt(argc, argv, "f:b:c:h:d:n:m:t:q:a:e:s:i:u:l:p:k:v:x:y:z:")) != -1) {
        switch (opt) {
        case 'n':
            CHUNK_SIZE = atoi(optarg);
            printf("Chunk Size = %d\n", CHUNK_SIZE);
            break;
        case 'b':
            link_bandwidth = atof(optarg);
            printf("Running with a link bandwidth of: %fGbps\n", link_bandwidth);
            break;
        case 'f':
            if (strlen(optarg) < 500) {
                strcpy(filename, optarg);
                strncpy(out_filename, filename, strlen(filename));
                strncat(out_filename, out_suffix, 4);
#ifdef RECORD_PACKETS
                printf("Writing switch packet data to switch.csv files\n");
                printf("open switch out tiles %s\n", filename);
                open_switch_outfiles(filename);
                open_host_outfiles(filename);
#endif
            }
            break;
        default:
            printf("Wrong command line argument\n");
            exit(1);
        }
    }

    timeslot_len = (pkt_size * 8) / link_bandwidth;
    printf("Running with a slot length of %fns\n", timeslot_len);

    per_hop_propagation_delay_in_timeslots = round((float)per_hop_propagation_delay_in_ns / (float)timeslot_len);
    per_sw_delay_in_timeslots = round((float)per_sw_delay_in_ns / (float)timeslot_len);
    printf("SW delay: %f ns (%d timeslots)\nPer hop propagation delay: %f ns (%d timeslots)\n",
        per_sw_delay_in_ns, per_sw_delay_in_timeslots, per_hop_propagation_delay_in_ns, per_hop_propagation_delay_in_timeslots);

    bytes_per_timeslot = (int)(timeslot_len * link_bandwidth / 8);
    printf("Bytes sent per timeslot %d\n", bytes_per_timeslot);

    DIR* dir = opendir("out/");
    if (dir) {
        closedir(dir);
    }
    else if (ENOENT == errno) {
        mkdir("out/", 0777);
    }
    else {
        printf("Could not open out directory.");
        exit(1);
    }

    read_tracefile(filename);
    out_fp = open_outfile(out_filename);
}
