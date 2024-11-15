#include "timeseries.h"

timeseries_t * create_timeseries() {
    timeseries_t * ts = malloc(sizeof(timeseries_t));
    MALLOC_TEST(ts, __LINE__);
    ts->max_val = 0;
    ts->curr_timeslot = 0;
    ts->max_timeslot = 256;
    ts->list = malloc(sizeof(int64_t) * ts->max_timeslot);
    MALLOC_TEST(ts->list, __LINE__);
    for (int i = 0; i < ts->max_timeslot; i++) {
        ts->list[i] = 0;
    }
    return ts;
}

void timeseries_add(timeseries_t * ts, int64_t val) {
    if (ts->curr_timeslot >= ts->max_timeslot) {
        ts->max_timeslot *= 2;
        int64_t * new_list = malloc(sizeof(int64_t) * ts->max_timeslot);
        MALLOC_TEST(ts, __LINE__);
        for (int i = 0; i < ts->max_timeslot; i++) {
            if (i < ts->max_timeslot / 2) {
                new_list[i] = ts->list[i];
            }
            else {
                new_list[i] = 0;
            }
        }
        free(ts->list);
        ts->list = new_list;
    }

    if (val >= ts->max_val) {
        ts->max_val = val;
    }
    ts->list[ts->curr_timeslot] = val;
    ts->curr_timeslot++;
}

void free_timeseries(timeseries_t * ts) {
    if (ts != NULL) {
        if (ts->list != NULL) {
            free(ts->list);
        }
        free(ts);
    }
}