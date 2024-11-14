#ifndef __SYSTEM_STATS_H__
#define __SYSTEM_STATS_H__

#include "params.h"
#include "tor.h"
#include "flow.h"

void print_network_tput(int64_t bytes, int64_t pkts, int64_t ns);
void print_cache_stats(int64_t cache_misses, int64_t cache_hits);
void print_tor_stats(tor_t *tors);
void print_delay_stats(float avg_delay_at_spine, float avg_max_delay_at_spine, float timeslot_len);
FILE *open_outfile(char *filename);
void write_to_outfile(FILE *fp, flow_t *flow, float timeslot_len, int bandwidth);
FILE *open_timeseries_outfile(char *filename);
#endif