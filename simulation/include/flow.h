#ifndef __FLOW_H__
#define __FLOW_H__

#include "params.h"
#include "arraylist.h"

typedef struct flow
{
    /*
    Calculate priority: memType * grantState. The smaller, the superior.

    flowType: Read from TraceFile at initialize_flow(). Aligned with PKT->pktType
        -1 = initial
        1: RREQ (highest priority)              = RREQ
        2: RESP
        3: WREQ                                 = WREQ
        100: net                                = NET

    grantState: given at runtime, but the first one is initialized by trace file.
        1 = waiting to send NOTIF
        -1 = notified, waiting for grant, CANNOT SEND
        10 = granted, can send
        100 = net, can send

        Possible product could be:
            2: RRESP-waiting to send NOTIF          = RNTF (initialize)
            -2: RRESP-notified, waiting for grant   = RWAIT
            20: RRESP-granted, can send             = RGRANTED

            3: WREQ-waiting to send NOTIF           = WNTF (initialize)
            -3: WREQ-notified, waiting for grant    = WWAIT
            30: WREQ-granted, can send              = WGRANTED

            100*100: net cansend                    = NETSEND
    */
    int flowType;
    int grantState;
    int quota; // The number of bytes that can be sent in current granted chunk. For Netflow, this is remaining burst.

    int rreq_bytes; // if this is a RREQ, then this means it is the request length.
    int grantTime;  // Time of the FIRST grant at ToR
    int notifTime;  // Time of notif at HOST.

    int8_t active;
    int8_t finished;
    int64_t flow_id;
    int64_t flow_size_bytes;
    int16_t src;
    int16_t dst;
    int64_t timeslot; // Written in tracefile, deciding when the flow should be activated

    int64_t start_timeslot; // The actual time of the first pkt from this flow being sent.
    int64_t finish_timeslot;
    int64_t timeslots_active; // The number of active timeslots
    int64_t pkts_sent;
    int64_t pkts_received;
    int pkts_dropped;
    int64_t bytes_sent;
    int64_t bytes_received;
    int64_t expected_runtime;
} flow_t;

flow_t *create_flow(int64_t, int64_t, int16_t, int16_t, int64_t);
void free_flow(flow_t *);

#endif
