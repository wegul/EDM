#ifndef __TIMESERIES__
#define __TIMESERIES__

#include <stdint.h>
#include "params.h"

typedef struct timeseries {
    int64_t * list;
    int64_t curr_timeslot;
    int64_t max_val;
    int64_t max_timeslot;
} timeseries_t;

timeseries_t * create_timeseries();
void timeseries_add(timeseries_t *, int64_t);
void free_timeseries(timeseries_t *);

#endif
