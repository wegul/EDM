#include "links.h"

links_t create_links()
{
    links_t self = (links_t)malloc(sizeof(struct links));
    MALLOC_TEST(self, __LINE__);

    for (int i = 0; i < NUM_OF_NODES; ++i)
    {
        for (int j = 0; j < NUM_OF_TORS; ++j)
        {
            self->host_to_tor_link[i][j] = create_link(i, j, LINK_CAPACITY);
        }
    }

    for (int i = 0; i < NUM_OF_TORS; ++i)
    {
        for (int j = 0; j < NUM_OF_SPINES; ++j)
        {
            self->tor_to_spine_link[i][j] = create_link(i, j, LINK_CAPACITY);
        }
    }

    for (int i = 0; i < NUM_OF_SPINES; ++i)
    {
        for (int j = 0; j < NUM_OF_TORS; ++j)
        {
            self->spine_to_tor_link[i][j] = create_link(i, j, LINK_CAPACITY);
        }
    }

    for (int i = 0; i < NUM_OF_TORS; ++i)
    {
        for (int j = 0; j < NUM_OF_NODES; ++j)
        {
            self->tor_to_host_link[i][j] = create_link(i, j, LINK_CAPACITY);
        }
    }

    return self;
}

void free_links(links_t self)
{
    if (self != NULL)
    {
        for (int i = 0; i < NUM_OF_NODES; ++i)
        {
            for (int j = 0; j < NUM_OF_TORS; ++j)
            {
                free_link(self->host_to_tor_link[i][j]);
            }
        }

        for (int i = 0; i < NUM_OF_TORS; ++i)
        {
            for (int j = 0; j < NUM_OF_SPINES; ++j)
            {
                free_link(self->tor_to_spine_link[i][j]);
            }
        }

        for (int i = 0; i < NUM_OF_SPINES; ++i)
        {
            for (int j = 0; j < NUM_OF_TORS; ++j)
            {
                free_link(self->spine_to_tor_link[i][j]);
            }
        }

        for (int i = 0; i < NUM_OF_TORS; ++i)
        {
            for (int j = 0; j < NUM_OF_NODES; ++j)
            {
                free_link(self->tor_to_host_link[i][j]);
            }
        }

        free(self);
    }
}
