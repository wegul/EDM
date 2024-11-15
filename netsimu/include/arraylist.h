#ifndef __ARRAYLIST__
#define __ARRAYLIST__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

typedef struct arraylist* arraylist_t;

arraylist_t create_arraylist();
void arraylist_add(arraylist_t, void*);
void* arraylist_get(arraylist_t, int64_t);
void arraylist_insert(arraylist_t, void*, int64_t);
void arraylist_update(arraylist_t, void*, int64_t);
void arraylist_remove(arraylist_t, int64_t);
int64_t arraylist_size(arraylist_t);
void free_arraylist(arraylist_t);

#endif
