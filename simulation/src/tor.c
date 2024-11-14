#include "tor.h"

tor_t create_tor(int16_t tor_index)
{
    tor_t self = (tor_t)malloc(sizeof(struct tor));
    MALLOC_TEST(self, __LINE__);
    memset(self, 0, sizeof(struct tor));
    self->tor_index = tor_index;
    self->ntf_cnt = 0;
    for (int i = 0; i < NODES_PER_RACK; ++i)
    {
        self->upstream_pkt_buffer[i] = create_buffer(TOR_UPSTREAM_BUFFER_LEN);      // recved from host
        self->downstream_send_buffer[i] = create_buffer(TOR_DOWNSTREAM_BUFFER_LEN); // pkts that will be sent to host
        self->upstream_mem_buffer[i] = create_buffer(TOR_DOWNSTREAM_MEMBUF_LEN);
        self->downstream_mem_buffer[i] = create_buffer(TOR_UPSTREAM_MEMBUF_LEN);
        self->downstream_mem_buffer_lock[i] = -1;
    }
    for (int i = 0; i < MAX_FLOW_ID; i++)
    {
        self->notif_queue[i] = malloc(sizeof(struct notification));
    }

    for (int i = 0; i < MAX_FLOW_ID; i++)
    {
        self->notif_queue[i]->reqFlowID = -1;
        self->notif_queue[i]->curQuota = 0;
        self->notif_queue[i]->remainingReqLen = -1;
        self->notif_queue[i]->sender = -1;
        self->notif_queue[i]->receiver = -1;
        self->notif_queue[i]->isRREQFirst = 0;
    }
    return self;
}

void free_tor(tor_t self)
{
    if (self != NULL)
    {

        for (int i = 0; i < NODES_PER_RACK; ++i)
        {
            free_buffer(self->upstream_pkt_buffer[i]);
            free_buffer(self->downstream_send_buffer[i]);
            free_buffer(self->upstream_mem_buffer[i]);
            free_buffer(self->downstream_mem_buffer[i]);
        }
        free(self);
    }
}
notif_t copy_notif(notif_t src)
{
    if (src == NULL)
    {
        // Source is NULL, return NULL to indicate no data to copy
        return NULL;
    }
    notif_t dest = malloc(sizeof(struct notification));
    if (dest == NULL)
    {
        // Memory allocation failed
        return NULL;
    }
    // Copying data from source to destination
    dest->remainingReqLen = src->remainingReqLen;
    dest->curQuota = src->curQuota;
    dest->reqFlowID = src->reqFlowID;
    dest->isGranted = src->isGranted;
    dest->sender = src->sender;
    dest->receiver = src->receiver;

    return dest;
}