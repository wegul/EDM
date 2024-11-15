#include "routing_table.h"

int create_routing_table(rnode_t * routing_table) {
    int i;
    for (i = 0; i < RTABLE_SIZE; i++) {
        routing_table[i].port = -1;
    }
    return i;
}

int hash(rnode_t * routing_table, int key) {
    // if (routing_table[key % RTABLE_SIZE].port == -1) {
    //     int spine = key % NUM_OF_SPINES;
    //     routing_table[key % RTABLE_SIZE].port = spine;
    // }
    // return routing_table[key % RTABLE_SIZE].port;
    return key % NUM_OF_SPINES;
}
