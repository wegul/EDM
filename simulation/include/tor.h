#ifndef __TOR_H__
#define __TOR_H__

#include "params.h"
#include "buffer.h"
#include "packet.h"
#include "routing_table.h"
#include "timeseries.h"
#include "memory.h"
struct notification
{
    // int reqType; // -1=invalid; 0=RREQ, 2=WREQ
    int remainingReqLen; // Total remaining # of bytes that will be send
    int curQuota;        // Remaining bytes of current distribution
    int reqFlowID;
    int isGranted; // Denote that this grant is sent out and that the quota has not run out yet.
    int sender;    // The sender of mem_msg. If RREQ, sender is pkt->dst_node, else is pkt->src_node
    int receiver;

    int isRREQFirst; // if is RREQ, generate 2 bloc.
};
typedef struct notification *notif_t;

notif_t copy_notif(notif_t src);

struct tor
{
    int16_t tor_index;
    int ntf_cnt;
    // mem buffer
    buffer_t *upstream_mem_buffer[NODES_PER_RACK];
    buffer_t *downstream_mem_buffer[NODES_PER_RACK];
    // packet buffer data structures
    buffer_t *upstream_pkt_buffer[NODES_PER_RACK];
    // packet sending data structures
    buffer_t *downstream_send_buffer[NODES_PER_RACK];
    notif_t notif_queue[MAX_FLOW_ID];
    int downstream_mem_buffer_lock[NODES_PER_RACK];
};
typedef struct tor *tor_t;

extern tor_t *tors;

tor_t create_tor(int16_t);
void free_tor(tor_t);
packet_t process_packets_up(tor_t tor, int16_t port, int64_t *cache_misses, int64_t *cache_hits, int sram_type);
packet_t process_packets_down(tor_t tor, int16_t port, int64_t *cache_misses, int64_t *cache_hits, int sram_type);
packet_t move_to_up_send_buffer(tor_t tor, int16_t port);
packet_t move_to_down_send_buffer(tor_t tor, int16_t port);
packet_t send_to_spine_baseline(tor_t, int16_t);
packet_t send_to_spine(tor_t, int16_t);
packet_t send_to_spine_dm(tor_t, int16_t, int64_t *, int64_t *);
packet_t send_to_spine_dram_only(tor_t tor, int16_t spine_id, int64_t *cache_misses);
packet_t send_to_host_baseline(tor_t, int16_t);
packet_t send_to_host(tor_t, int16_t);
packet_t send_to_host_dram_only(tor_t tor, int16_t host_within_tor, int64_t *cache_misses);
int64_t tor_up_buffer_bytes(tor_t, int);
int64_t tor_down_buffer_bytes(tor_t, int);
int64_t *linearize_tor_downstream_queues(tor_t, int *);

#endif
