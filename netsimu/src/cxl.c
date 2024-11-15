#include "driver.h"

// Default values for simulation
static int pkt_size = BLK_SIZE;    // in bytes
static float link_bandwidth = 100; // in Gbps
static float timeslot_len;         // in ns
static int bytes_per_timeslot = 8;
float per_hop_propagation_delay_in_ns = 0.64;
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

volatile int64_t num_of_flows_finished = 0; // extern var
volatile int64_t total_flows_started = 0;   // extern var
int CHUNK_SIZE = 256;
volatile int64_t max_bytes_rcvd = 1000000;

// Output files
FILE* out_fp = NULL;
FILE* sw_queue_fp = NULL;
FILE* tor_outfiles[NUM_OF_RACKS];
FILE* host_outfiles[NUM_OF_NODES];

// Network
node_t* nodes;
tor_t* tors;
links_t links;
flowlist_t* flowlist;
int resp2req[MAX_FLOW_ID] = { 0 };
int req2resp[MAX_FLOW_ID] = { 0 };

int tor_credit[NODES_PER_RACK] = { 0 }; // Switch port send capacity = host recv capacity.
int host_credit[NODES_PER_RACK] = { 0 };
int credit_thres = 2 * 2048 * BLK_SIZE;
int credit_bound = 2 * 8192 * BLK_SIZE;

int host2tor_upd[NODES_PER_RACK] = { 0 };
int tor2host_upd[NODES_PER_RACK] = { 0 };

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
                // printf("activate flow: %d, curr: %d\n", flow->flow_id, curr_timeslot);
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
            packet_t peek_pkt = NULL;
            for (int i = 0; i < TOR_UPSTREAM_MEMBUF_LEN; i++) {
                peek_pkt = (packet_t)buffer_peek(tor->upstream_mem_buffer[j], i);
                if (!peek_pkt) {
                    break;
                }
                int dst_host = peek_pkt->dst_node;
                int pkt_size = peek_pkt->size;
                if (tor_credit[dst_host] < 1) {
                    continue;
                }
                packet_t pkt = (packet_t)buffer_remove(tor->upstream_mem_buffer[j], i);
                i--;
                pkt_recv(tor->downstream_mem_buffer[dst_host], pkt);
                tor_credit[dst_host] -= 1;
            }
        }

        /*---------------------------------------------------------------------------*/
        // HOST -- SEND
        /*---------------------------------------------------------------------------*/

        for (int i = 0; i < NUM_OF_NODES; ++i) {
            node_t node = nodes[i];
            int16_t node_index = node->node_index;

            if (tor_credit[node_index] < credit_thres && !host2tor_upd[node_index]) // if tor has low send credit, host should update but only once.
            {
                host2tor_upd[node_index] = 1;
                host_credit[node_index] -= 1;
                packet_t upd_pkt = create_packet(node_index, -1, -1, BLK_SIZE, -1, packet_counter++);
                upd_pkt->pktType = GRT_TYPE;
                upd_pkt->reqLen = credit_bound - tor->downstream_mem_buffer[node_index]->num_elements;
                upd_pkt->time_when_transmitted_from_src = curr_timeslot;
                upd_pkt->time_to_dequeue_from_link = curr_timeslot + per_hop_propagation_delay_in_timeslots;
                link_enqueue(links->host_to_tor_link[node_index][0], upd_pkt);
                continue;
            }
            if (host_credit[node_index] < 1) {
                continue;
            }
            flow_t* peek_flow0 = (flow_t*)buffer_peek(node->active_flows, 0);
            if (node->current_flow == NULL) // Curr is null, should select a flow
            {
                if (peek_flow0) {
                    node->current_flow = buffer_get(node->active_flows);
                }
            }
            else if (peek_flow0 && peek_flow0->flowType != RREQ_TYPE) // check if there are other RREQ
            {
                for (int j = 1; j < node->active_flows->num_elements; j++) {
                    flow_t* peek_flow1 = (flow_t*)buffer_peek(node->active_flows, j);
                    if (peek_flow1 && peek_flow1->flowType == RREQ_TYPE) {
                        flow_t* new_flow = (flow_t*)buffer_remove(node->active_flows, j);
                        assert("Put back current flow" && -1 != buffer_put(node->active_flows, node->current_flow));
                        node->current_flow = new_flow;
                        break;
                    }
                }
            }

            flow_t* flow = node->current_flow;

            // Now that flow is selected, start sending packet
            if (flow) {
                int16_t src_node = flow->src;
                int16_t dst_node = flow->dst;
                int64_t flow_id = flow->flow_id;
                // send packet

                int64_t size = BLK_SIZE;
                int64_t flow_bytes_remaining = flow->flow_size_bytes - flow->bytes_sent;

                packet_t pkt = NULL;

                // If no retrans and there are new packets need to be sent (some packets are not received/acked)
                if (flow_bytes_remaining > 0) {
                    // Create packet
                    pkt = create_packet(src_node, dst_node, flow_id, size, node->seq_num[flow_id], packet_counter++);
                    node->seq_num[flow_id] += size;
                    if (flow->flowType == RREQ_TYPE) // RREQ, check if it is header (first). The first RREQ should tell the total num of flow_size. The receiver will chunk it.
                    {
                        if (flow->bytes_sent == 0) // is Header
                        {
                            pkt->pktType = NET_TYPE; // If it is the first one, just send it as regular traffic.
                        }
                        else {
                            flow->grantState = GRANTED_STATE; // RREQ has to finish, cannot be preempted.
                            pkt->pktType = RREQ_TYPE; // RREQ header
                            pkt->reqLen = flow->rreq_bytes;
                            flow->notifTime = curr_timeslot;
                        }
                    }
                    else {
                        pkt->pktType = NET_TYPE;
                    }

                    // Update flow state
                    if (flow->active == 0) {
                        printf("Flow %d (%d) started with delay %d, remaining: %d\n", flow->flow_id, flow->flowType, curr_timeslot - flow->timeslot, flowlist->num_flows - total_flows_started);
                        flowlist->active_flows++;
                        flow->start_timeslot = curr_timeslot;
                        total_flows_started++;
                    }
                    flow->active = 1;
                    flow->bytes_sent += size;
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
                    pkt->time_to_dequeue_from_link = curr_timeslot + per_hop_propagation_delay_in_timeslots;
                    link_enqueue(links->host_to_tor_link[node_index][0], pkt);
                    host_credit[node_index] -= 1;
                }
                if (flow->flow_size_bytes - flow->bytes_sent < 1) // Finished sending...
                {
                    node->current_flow = NULL;
                }
            }
        }

        /*---------------------------------------------------------------------------*/
        // ToR -- SEND TO HOST
        /*---------------------------------------------------------------------------*/
        // send to each host
        for (int tor_port = 0; tor_port < TOR_PORT_COUNT_LOW; ++tor_port) {
            if (host_credit[tor_port] < credit_thres && !tor2host_upd[tor_port]) {

                packet_t upd_pkt = create_packet(-1, tor_port, -1, BLK_SIZE, -1, packet_counter++);
                upd_pkt->pktType = GRT_TYPE;
                upd_pkt->reqLen = credit_bound - tor->upstream_mem_buffer[tor_port]->num_elements; // It should be the current capacity.
                upd_pkt->time_when_transmitted_from_src = curr_timeslot;
                upd_pkt->time_to_dequeue_from_link = curr_timeslot + per_hop_propagation_delay_in_timeslots;
                if (upd_pkt->reqLen > 0) {
                    tor2host_upd[tor_port] = 1;
                    tor_credit[tor_port] -= 1;
                    link_enqueue(links->tor_to_host_link[tor_index][tor_port], upd_pkt);
                    continue;
                }
            }
            packet_t pkt = (packet_t)buffer_get(tor->downstream_mem_buffer[tor_port]);
            if (pkt) {
                // printf("tor sent (%d), cnt: %d %d-%d, flowid: %d, buf: %d/%d, curr: %d\n",
                //        pkt->pktType, pkt->pkt_id, pkt->src_node, pkt->dst_node, pkt->flow_id, tor->downstream_mem_buffer[tor_port]->num_elements, tor->downstream_mem_buffer[tor_port]->size, curr_timeslot);
                pkt->time_to_dequeue_from_link = curr_timeslot + per_hop_propagation_delay_in_timeslots;
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
            int link_num = links->host_to_tor_link[src_host][tor_index]->fifo->num_elements;
            for (int i = 0; i < link_num; i++) {
                peek_pkt = (packet_t)link_peek(links->host_to_tor_link[src_host][tor_index], i);
                if (peek_pkt != NULL && peek_pkt->time_to_dequeue_from_link == curr_timeslot) {
                    pkt = link_get(links->host_to_tor_link[src_host][tor_index], i);
                    if (pkt->pktType == GRT_TYPE) // update fc
                    {
                        tor_credit[tor_port] = pkt->reqLen;
                        host2tor_upd[tor_port] = 0;
                    }
                    else {
                        assert("tor upstream recv drop" && -1 != pkt_recv(tor->upstream_mem_buffer[tor_port], pkt));
                    }
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
            if (peek_pkt != NULL && peek_pkt->time_to_dequeue_from_link == curr_timeslot) {
                pkt = link_get(links->tor_to_host_link[0][node_index], 0);
                assert(pkt->dst_node == node_index);
                if (pkt->pktType == GRT_TYPE) {
                    host_credit[node_index] = pkt->reqLen;
                    tor2host_upd[node_index] = 0;
                    // printf("host %d recv upd %d\n", node_index, host_credit[node_index]);
                    continue;
                }
                flow_t* flow = flowlist->flows[pkt->flow_id];
                assert(flow != NULL);
                if (pkt->pktType == RREQ_TYPE) {
                    if (flow->grantTime < 0) // Check if it is the first time grant
                    {
                        int rresp_flow_size = flow->rreq_bytes;
                        flow_t* rresp_flow = create_flow(flowlist->num_flows, rresp_flow_size, node->node_index /* DST send to Requester*/, flow->src, curr_timeslot + 1);
                        req2resp[flow->flow_id] = rresp_flow->flow_id;
                        resp2req[rresp_flow->flow_id] = flow->flow_id;
                        rresp_flow->flowType = RRESP_TYPE;
                        rresp_flow->grantState = GRANTED_STATE; // initial as granted
                        if (rresp_flow_size > CHUNK_SIZE) {
                            rresp_flow->expected_runtime = ((rresp_flow->flow_size_bytes) / CHUNK_SIZE + 1) * CHUNK_SIZE / BLK_SIZE +
                                4 * per_hop_propagation_delay_in_timeslots + flow->expected_runtime; // Additional BDP
                        }
                        else {
                            rresp_flow->expected_runtime = rresp_flow->flow_size_bytes / BLK_SIZE + flow->expected_runtime;
                        }
                        flow->grantTime = curr_timeslot;
                        rresp_flow->notifTime = flow->timeslot; // For stat/debug purpose, the FCT of RRESP should be sum of its RREQ and itself.
                        rresp_flow->grantTime = curr_timeslot;
                        assert("Too many flows!" && flowlist->num_flows - 1 < MAX_FLOW_ID);
                        add_flow(flowlist, rresp_flow);
                    }
                    else {
                        assert(req2resp[flow->flow_id] >= 0);
                    }
                }
                flow->pkts_received++;
                flow->bytes_received += BLK_SIZE;
                total_bytes_rcvd += BLK_SIZE;
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
            }
            free_packet(pkt);
        }

        /*---------------------------------------------------------------------------*/
        // State updates before next iteration
        /*---------------------------------------------------------------------------*/
        int no_flows_left = 1;
        if (flowlist->active_flows < 1) {
            for (int i = 0; i < flowlist->num_flows; i++) {
                flow_t* flow = flowlist->flows[i];
                if (flow != NULL && flow->finished == 0) {
                    no_flows_left = 0;
                }
            }
        }
        if (num_of_flows_finished >= 1500) {
            printf("Finished all flows\n\n");
            terminate0 = 1;
        }
        if (total_flows_started >= flowlist->num_flows) {
            printf("\n======== All %d flows started ========\n\n", (int)total_flows_started);
            terminate1 = 1;
        }
        if (terminate0 || terminate1) {
            int completed_flows = 0;
            float flow_completion_times[flowlist->num_flows];
            float slowdowns[flowlist->num_flows];
            float avg_slowdown = 0;
            for (int i = 0; i < flowlist->num_flows; i++) {
                if (flowlist->flows[i]->finished > 0) {
                    flow_completion_times[completed_flows] = flowlist->flows[i]->finish_timeslot - flowlist->flows[i]->timeslot;
                    avg_flow_completion_time += flowlist->flows[i]->finish_timeslot - flowlist->flows[i]->timeslot;
                    //Only count RRES&WREQ
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
    for (int i = 0; i < NODES_PER_RACK; i++) {
        host_credit[i] = credit_bound;
        tor_credit[i] = credit_bound;
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
            if (flowType == RREQ_TYPE) {
                new_flow->quota = flow_size_bytes;
                if (rreq_bytes > CHUNK_SIZE) {
                    new_flow->expected_runtime += rreq_bytes / BLK_SIZE;
                }
            }
            else {
                if (flow_size_bytes > CHUNK_SIZE) {
                    new_flow->expected_runtime += (CHUNK_SIZE * 4) / BLK_SIZE;
                }
                else {
                    new_flow->expected_runtime += 2 * per_hop_propagation_delay_in_timeslots;
                }
            }
        }
        else {
            new_flow->quota = ETH_MTU;
            new_flow->grantState = NET_STATE;
            new_flow->expected_runtime = flow_size_bytes / BLK_SIZE + 2 * per_hop_propagation_delay_in_timeslots + per_sw_delay_in_timeslots;
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
int cmp_flow(const void* a, const void* b) {
    flow_t* flow1 = *(flow_t**)a;
    flow_t* flow2 = *(flow_t**)b;
    // Granted < Notif < Net < Waiting
    if (flow1->grantState < flow2->grantState) {
        return -1;
    }
    else if (flow1->grantState > flow2->grantState) {
        return 1;
    }
    else // tie breaker
    {
        // RREQ < RRESP < WREQ < NET
        if (flow1->flowType == RREQ_TYPE) {
            return -1;
        }
        else if (flow2->flowType == RREQ_TYPE) {
            return 1;
        }
        else {
            // if (flow1->flowType == RREQ_TYPE) // They are both RREQ
            // {
            //     if (flow1->rreq_bytes < flow2->rreq_bytes)
            //     {
            //         return -1;
            //     }
            //     else if (flow1->rreq_bytes > flow2->rreq_bytes)
            //     {
            //         return 1;
            //     }
            // }
            if (flow1->flowType == WREQ_TYPE || flow1->flowType == RRESP_TYPE) // Both WREQ, and both are Notified.
            {
                if (flow1->flow_size_bytes - flow1->bytes_sent < flow2->flow_size_bytes - flow2->bytes_sent) {
                    return -1;
                }
                else if (flow1->flow_size_bytes - flow1->bytes_sent > flow2->flow_size_bytes - flow2->bytes_sent) {
                    return 1;
                }
            }
            if (flow1->timeslot < flow2->timeslot) {
                return -1;
            }
            else if (flow1->timeslot > flow2->timeslot) {
                return 1;
            }
            else {
                return 0;
            }
        }
    }
}
