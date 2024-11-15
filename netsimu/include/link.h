#ifndef __LINK_H__
#define __LINK_H__

#include "params.h"
#include "buffer.h"
typedef struct link *link_t;
struct link
{
    int16_t src_node;
    int16_t dst_node;
    buffer_t *fifo;
    void *ipg_data;
};

link_t create_link(int16_t, int16_t, int32_t);
void link_enqueue(link_t, void *);
void *link_dequeue(link_t);
void *link_peek(link_t, int32_t);
void *link_get(link_t, int32_t);
void free_link(link_t);
void ipg_send(link_t, void *);
void *ipg_recv(link_t);
void *ipg_peek(link_t);

#endif
