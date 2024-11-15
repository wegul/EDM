#include "flowlist.h"

flowlist_t * create_flowlist() {
    flowlist_t * self = NULL;
    self = (flowlist_t *) malloc(sizeof(flowlist_t));
    MALLOC_TEST(self, __LINE__);
    self->num_flows = 0;
    self->active_flows = 0;
    for (int i = 0; i < MAX_FLOW_ID; i++) {
        self->flows[i] = NULL;
    }
    return self;
}

void add_flow(flowlist_t * self, flow_t * flow) {
    if (self != NULL && flow != NULL) {
        self->num_flows++;
        int64_t flow_id = flow->flow_id;
        if (self->flows[flow_id] != NULL) {
            free(self->flows[flow_id]);
        }
        self->flows[flow_id] = flow;
    }
}

flow_t * check_flow(flowlist_t * self, int64_t flow_id) {
    if (self != NULL) {
        return self->flows[flow_id];
    }
    else {
        return NULL;
    }
}

void free_flowlist(flowlist_t * self) {
    if (self != NULL) {
        for (int i = 0; i < MAX_FLOW_ID; i++) {
            if (self->flows[i] != NULL) {
                free(self->flows[i]);
            }
        }
        free(self);
    }
}