#ifndef __LINKS_H__
#define __LINKS_H__

#include "params.h"
#include "link.h"

struct links
{
    link_t host_to_tor_link[NUM_OF_NODES][NUM_OF_TORS];
    link_t tor_to_spine_link[NUM_OF_TORS][NUM_OF_SPINES];
    link_t spine_to_tor_link[NUM_OF_SPINES][NUM_OF_TORS];
    link_t tor_to_host_link[NUM_OF_TORS][NUM_OF_NODES];
};

typedef struct links *links_t;

extern links_t links;

links_t create_links();
void free_links(links_t);

#endif
