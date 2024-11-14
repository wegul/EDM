#ifndef __FLOWLIST_H__
#define __FLOWLIST_H__

#include "params.h"
#include "flow.h"

typedef struct flowlist
{
    int64_t num_flows;
    int64_t active_flows;
    flow_t *flows[MAX_FLOW_ID];
} flowlist_t;

flowlist_t *create_flowlist();
void add_flow(flowlist_t *, flow_t *);
flow_t *check_flow(flowlist_t *, int64_t);
void free_flowlist(flowlist_t *);

#endif
