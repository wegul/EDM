#include "node.h"

node_t create_node(int16_t node_index)
{
    node_t self = (node_t)malloc(sizeof(struct node));
    MALLOC_TEST(self, __LINE__);

    self->node_index = node_index;

    self->active_flows = create_buffer(MAX_FLOW_ID);
    self->current_flow = NULL;

    for (int i = 0; i < MAX_FLOW_ID; ++i)
    {
        self->seq_num[i] = 0;
        self->ack_num[i] = 0;
        self->last_acked[i] = 0;
        self->last_ack_time[i] = 0;
        for (int j = 0; j < ECN_WIDTH; j++)
        {
            self->ecn_marks[i][j] = 0;
        }
        self->ecn_idx[i] = 0;
        self->cwnd[i] = CWND_START;
        self->ssthresh[i] = SSTHRESH_START;
        self->acks_since_last_cwnd_increase[i] = 0;
    }

    return self;
}

void track_ecn(node_t self, int32_t flow_id, int16_t ecn_mark)
{
    // Mark ECN
    self->ecn_marks[flow_id][self->ecn_idx[flow_id]] = ecn_mark;
    // CWND reduction due to fraction of ECN flags
    if (ecn_mark > 0)
    {
        int count = 0;
        for (int i = 0; i < ECN_WIDTH; i++)
        {
            count += self->ecn_marks[flow_id][i];
        }

        self->cwnd[flow_id] -= self->cwnd[flow_id] * count / ECN_WIDTH;
        if (self->cwnd[flow_id] < 1)
        {
            self->cwnd[flow_id] = 1;
        }
#ifdef DEBUG_DCTCP
        printf("node %d reduced flow %d cwnd to %d with %d/8 ECN markers\n", (int)self->node_index, (int)flow_id, (int)self->cwnd[flow_id], count);
#endif
    }
    else
    {
        // Slow start CWND increase
        if (self->cwnd[flow_id] < self->ssthresh[flow_id])
        {
            self->cwnd[flow_id] *= 2;
#ifdef DEBUG_DCTCP
            printf("node %d increased flow %d cwnd to %d (SLOW START)\n", (int)self->node_index, (int)flow_id, (int)self->cwnd[node]);
#endif
        }
        // Normal CWND increase: next increse 1 when current cwnd is cleared
        else
        {
            self->acks_since_last_cwnd_increase[flow_id]++;
            if (self->acks_since_last_cwnd_increase[flow_id] == self->cwnd[flow_id])
            {
                self->cwnd[flow_id]++;
                self->acks_since_last_cwnd_increase[flow_id] = 0;
#ifdef DEBUG_DCTCP
                printf("node %d increased flow %d cwnd to %d\n", (int)self->node_index, (int)flow_id, (int)self->cwnd[node]);
#endif
            }

#ifdef DEBUG_DCTCP
            printf("node %d received an ack for flow %d; %d more until next cwnd increase\n", (int)self->node_index, (int)flow_id, (int)(self->cwnd[node] - self->acks_since_last_cwnd_increase[node]));
#endif
        }
    }

    self->ecn_idx[flow_id]++;
    if (self->ecn_idx[flow_id] >= ECN_WIDTH)
    {
        self->ecn_idx[flow_id] = 0;
    }
}

void free_node(node_t self)
{
    if (self != NULL)
    {
        // Flows should already be freed by freeing the flowlist
        // free_buffer(self->active_flows);
        free(self->active_flows->buffer);
        free(self->active_flows);
        free(self);
    }
}
