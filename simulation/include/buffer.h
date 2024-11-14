#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define NULL_TEST(ptr, line_num)                                             \
    if (ptr == NULL)                                                         \
    {                                                                        \
        printf("error: %s:%d Null pointer exception\n", __FILE__, line_num); \
        exit(0);                                                             \
    }

#define MALLOC_TEST(ptr, line_num)                                    \
    if (ptr == NULL)                                                  \
    {                                                                 \
        printf("error: %s:%d malloc() failed\n", __FILE__, line_num); \
        exit(0);                                                      \
    }

typedef struct buffer
{
    int32_t size;
    int32_t num_elements;
    void **buffer;
} buffer_t;

typedef struct buff_node
{
    int64_t val;
} buff_node_t;

buffer_t *create_buffer(int32_t);
int8_t buffer_insert(buffer_t *, void *, int32_t);
int8_t buffer_put(buffer_t *, void *);
void *buffer_get(buffer_t *);
void *buffer_peek(buffer_t *, int32_t);
void *buffer_remove(buffer_t *, int32_t);
void print_buffer(buffer_t *);
void buffer_clear(buffer_t *);
void free_buffer(buffer_t *);

#endif