#include "system_stats.h"

// void print_system_stats(spine_t *spines, tor_t *tors, int64_t total_bytes, int64_t total_pkts, int64_t ns, int64_t cache_misses, int64_t cache_hits, float avg_delay_at_spine, float avg_max_delay_at_spine, float timeslot_len)
// {
//     // print_spine_stats(spines);
//     // print_tor_stats(tors);
//     print_network_tput(total_bytes, total_pkts, ns);
//     // print_cache_stats(cache_misses, cache_hits);
//     print_per_cache_stats(spines, tors);
//     print_delay_stats(avg_delay_at_spine, avg_max_delay_at_spine, timeslot_len);
// }

void print_network_tput(int64_t total_bytes, int64_t total_pkts, int64_t ns) {
    printf("Total bytes received: %ld B\n", total_bytes);
    printf("Full Network Throughput: %0.2f Gbps\n", (double)total_bytes * 8 / ns);
    printf("Packets per ns: %0.2f p pns\n", (double)total_pkts / ns);
}


void print_delay_stats(float avg_delay_at_spine, float avg_max_delay_at_spine, float timeslot_len) {
    printf("Avg delay at spine: %0.3f timeslots %0.3f ns\n", avg_delay_at_spine, (avg_delay_at_spine * timeslot_len));
    printf("Avg max delay at spine: %0.3f timeslots %0.3f ns\n", avg_max_delay_at_spine, (avg_max_delay_at_spine * timeslot_len));
}

FILE* open_outfile(char* filename) {
    char out_filename[520] = "out/";
    // char out_filename[520] = "./debugout.csv";
    strncat(out_filename, filename, 500);
    FILE* out_fp = fopen(out_filename, "w");
    assert(out_fp != NULL);
    fprintf(out_fp, "FlowID,FlowType,Src,Dst,FlowSize,Create,NotifTime,GrantTime,Start,Finish,FCT,Slowdown\n");
    return out_fp;
}

void write_to_outfile(FILE* fp, flow_t* flow, float timeslot_len, int bandwidth) {
    assert(fp != NULL);
    double sec_per_timeslot = timeslot_len / 1e9;

    int flow_id = (int)flow->flow_id;
    int src = (int)flow->src;
    int dst = (int)flow->dst;
    int bytes_received = (int)flow->bytes_received;
    // double flow_completion = (double)flow->finish_timeslot * sec_per_timeslot;
    int start_time = flow->start_timeslot;
    int finish_time = flow->finish_timeslot;
    int flow_completion = flow->finish_timeslot - flow->timeslot;
    int pkts_dropped = flow->pkts_dropped;
    if (flow->flowType == RRESP_TYPE) {
        flow_completion = flow->finish_timeslot - flow->notifTime;
    }
    else if (flow->flowType == NET_TYPE && flow->notifTime >= 0) {
        flow_completion = flow->finish_timeslot - flow->notifTime;
    }

    double slowdown = max(1, (double)(flow_completion) / (double)(flow->expected_runtime));
    double tput = (double)flow->bytes_received * 8 / (flow->timeslots_active * timeslot_len * 1.0);
    fprintf(fp, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.2f\n",
        flow_id, flow->flowType, src, dst, bytes_received, flow->timeslot, flow->notifTime, flow->grantTime, start_time, finish_time, flow_completion, slowdown);

    fflush(fp);
}
