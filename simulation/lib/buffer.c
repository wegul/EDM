#include "buffer.h"

buffer_t *create_buffer(int32_t size)
{
    buffer_t *self = NULL;
    if (size > 0)
    {
        self = (buffer_t *)malloc(sizeof(buffer_t));
        MALLOC_TEST(self, __LINE__);
        self->buffer = (void **)malloc(size * sizeof(void *));
        MALLOC_TEST(self->buffer, __LINE__);
        for (int i = 0; i < size; i++)
        {
            self->buffer[i] = NULL;
        }
        self->size = size;
        self->num_elements = 0;
    }

    return self;
}

int8_t buffer_insert(buffer_t *self, void *element, int32_t index)
{
    if (self != NULL && element != NULL && index <= self->num_elements)
    {
        self->num_elements++;
        if (self->num_elements > self->size)
        {
            self->size *= 2;
            void **new_buffer = NULL;
            new_buffer = (void **)malloc(self->size * sizeof(void *));
            MALLOC_TEST(new_buffer, __LINE__);
            for (int i = 0; i < self->size; i++)
            {
                if (i < self->size / 2)
                {
                    new_buffer[i] = self->buffer[i];
                }
                else
                {
                    new_buffer[i] = NULL;
                }
            }
            free(self->buffer);
            self->buffer = new_buffer;
        }
        for (int i = self->num_elements - 1; i > index; i--)
        {
            self->buffer[i] = self->buffer[i - 1];
        }
        self->buffer[index] = element;
        return 0;
    }
    return -1;
}

int8_t buffer_put(buffer_t *self, void *element)
{
    return buffer_insert(self, element, self->num_elements);
}
int8_t pkt_recv(buffer_t *self, void *element)
{
    return buffer_insert(self, element, self->num_elements);
}

void *buffer_remove(buffer_t *self, int32_t index)
{
    void *element = NULL;
    if (self != NULL && self->num_elements > index)
    {
        element = self->buffer[index];
        self->num_elements--;
        int i = index;
        for (i = index; i < self->num_elements; i++)
        {
            self->buffer[i] = self->buffer[i + 1];
        }
        self->buffer[i] = NULL;
    }
    return element;
}

void *buffer_get(buffer_t *self)
{
    return buffer_remove(self, 0);
}

void *buffer_peek(buffer_t *self, int32_t index)
{
    if (self == NULL)
    {
        return NULL;
    }

    if (index >= self->num_elements || index < 0)
    {
        return NULL;
    }
    void *element = self->buffer[index];
    return element;
}

void print_buffer(buffer_t *self)
{
    printf("Printing buffer of capacity %d / %d\n", self->num_elements, self->size);
    for (int i = 0; i < self->size; i++)
    {
        if (self->buffer[i] != NULL)
        {
            printf("%d, ", *((int *)self->buffer[i]));
        }
    }
    printf("\n");
}

void buffer_clear(buffer_t *self)
{
    if (self != NULL && self->buffer != NULL)
    {
        for (int i = 0; i < self->size; i++)
        {
            if (self->buffer[i] != NULL)
            {
                free(self->buffer[i]);
            }
            self->buffer[i] = NULL;
        }
        self->num_elements = 0;
    }
}

void free_buffer(buffer_t *self)
{
    if (self == NULL)
    {
        return;
    }
    buffer_clear(self);
    free(self->buffer);
    free(self);
}