#include "link.h"
link_t create_link(int16_t src_node, int16_t dst_node, int32_t capacity)
{
    link_t self = (link_t)malloc(sizeof(struct link));
    MALLOC_TEST(self, __LINE__);
    self->src_node = src_node;
    self->dst_node = dst_node;
    self->fifo = create_buffer(capacity);
    self->ipg_data = NULL;
    return self;
}

void link_enqueue(link_t self, void *element)
{
    NULL_TEST(self, __LINE__);
    assert(buffer_put(self->fifo, element) != -1);
}

void *link_dequeue(link_t self)
{
    NULL_TEST(self, __LINE__);
    return buffer_get(self->fifo);
}
void *link_get(link_t self, int32_t index)
{
    NULL_TEST(self, __LINE__);
    return buffer_remove(self->fifo, index);
}

void *link_peek(link_t self, int32_t index)
{
    NULL_TEST(self, __LINE__);
    return buffer_peek(self->fifo, index);
}

void free_link(link_t self)
{
    if (self != NULL)
    {
        free_buffer(self->fifo);
        if (self->ipg_data != NULL)
        {
            free(self->ipg_data);
        }
        free(self);
    }
}

void ipg_send(link_t self, void *data)
{
    if (self->ipg_data != NULL)
    {
        free(self->ipg_data);
        self->ipg_data = NULL;
    }
    self->ipg_data = data;
}

void *ipg_recv(link_t self)
{
    void *data = self->ipg_data;
    self->ipg_data = NULL;
    return data;
}

void *ipg_peek(link_t self)
{
    return self->ipg_data;
}