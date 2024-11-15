#include "flow.h"

flow_t *create_flow(int64_t flow_id, int64_t flow_size_bytes, int16_t host_index, int16_t dst_index, int64_t timeslot)
{
    flow_t *self = (flow_t *)malloc(sizeof(flow_t));
    MALLOC_TEST(self, __LINE__);
    self->grantTime = -1;
    self->notifTime = -1;
    self->quota = -1;
    self->active = 0;
    self->finished = 0;
    self->flow_id = flow_id;
    self->flow_size_bytes = flow_size_bytes;
    self->src = host_index;
    self->dst = dst_index;
    self->timeslot = timeslot;
    self->start_timeslot = 0;
    self->finish_timeslot = -1;
    self->timeslots_active = 0;
    self->pkts_sent = 0;
    self->pkts_received = 0;
    self->pkts_dropped = 0;
    self->bytes_sent = 0;
    self->bytes_received = 0;
    self->expected_runtime = 0;

    return self;
}

void free_flow(flow_t *self)
{
    if (self != NULL)
    {
        free(self);
    }
}
