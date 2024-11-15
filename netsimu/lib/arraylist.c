#include "arraylist.h"

#define NULL_TEST(ptr, line_num) \
    if (ptr == NULL) \
        {printf("error: %s:%d Null pointer exception\n", __FILE__, line_num); \
        exit(0);}

#define MALLOC_TEST(ptr, line_num) \
    if (ptr == NULL) \
        {printf("error: %s:%d malloc() failed\n", __FILE__, line_num); \
        exit(0);}

struct arraylist {
    void** list;
    int64_t num_of_items;
    int64_t length;
};

arraylist_t create_arraylist()
{
    arraylist_t self = (arraylist_t) malloc(sizeof(struct arraylist));
    MALLOC_TEST(self, __LINE__);
    self->list = (void**) malloc(sizeof(void*));
    MALLOC_TEST(self->list, __LINE__);
    self->num_of_items = 0;
    self->length = 1;
    return self;
}

void arraylist_add(arraylist_t self, void* item)
{
    NULL_TEST(self, __LINE__);

    if (self->length != 0) {
        if (self->num_of_items == self->length) {
            self->list = realloc(self->list, 2 * self->length * sizeof(void*));
            MALLOC_TEST(self->list, __LINE__);
            self->length *= 2;
        }

        self->list[self->num_of_items] = item;
        ++(self->num_of_items);
    }
}

void* arraylist_get(arraylist_t self, int64_t index)
{
    NULL_TEST(self, __LINE__);

    if (self->num_of_items == 0) return NULL;

    assert(index >= 0 && index < self->num_of_items);

    return (self->list[index]);
}

void arraylist_insert(arraylist_t self, void* item, int64_t index)
{
    NULL_TEST(self, __LINE__);

    if (item == NULL) return;

    //arraylist is empty
    if (self->num_of_items == 0 && index == 0) {
        arraylist_add(self, item);
        return;
    }

    assert(index >= 0 && index < self->num_of_items);

    arraylist_add(self, NULL);
    memmove((self->list + index + 1),
            (self->list + index),
            ((self->num_of_items - index - 1) * sizeof(void*)));
    self->list[index] = item;
}

void arraylist_update(arraylist_t self, void* item, int64_t index)
{
    NULL_TEST(self, __LINE__);

    if (item == NULL) return;

    assert(index >= 0 && index < self->num_of_items);

    self->list[index] = item;
}

void arraylist_remove(arraylist_t self, int64_t index)
{
    NULL_TEST(self, __LINE__);

    assert(index >= 0 && index < self->num_of_items);

    if (index < self->num_of_items - 1) {
        memmove((self->list + index),
                (self->list + index + 1),
                ((self->num_of_items - index - 1) * sizeof(void*)));
    }

    --(self->num_of_items);

    if (self->num_of_items <= self->length/4) {
        if (self->length/2 > 0) {
            self->list = realloc(self->list, (self->length/2) * sizeof(void*));
            MALLOC_TEST(self->list, __LINE__);
            self->length /= 2;
        } else {
            self->list = realloc(self->list, sizeof(void*));
            MALLOC_TEST(self->list, __LINE__);
            self->length = 1;
        }
    }
}

int64_t arraylist_size(arraylist_t self)
{
    NULL_TEST(self, __LINE__);

    return self->num_of_items;
}

void free_arraylist(arraylist_t self)
{
    if (self != NULL) {
        free(self->list);
        free(self);
    }
}
