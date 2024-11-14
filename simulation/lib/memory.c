#include "memory.h"

lru_node_t * create_lru_node(int64_t flow_id, int64_t val) {
    lru_node_t * node = malloc(sizeof(lru_node_t));
    MALLOC_TEST(node, __LINE__);
    node->flow_id = flow_id;
    node->val = val;
    node->next = NULL;

    return node;
}

void push(lru_node_t ** head_ptr, lru_node_t * node) {
    node->next = (*head_ptr);
    (*head_ptr) = node;
}

lru_node_t * pop(lru_node_t ** head_ptr) {
    if (*head_ptr == NULL) {
        return NULL;
    }

    lru_node_t * node = (*head_ptr);
    lru_node_t * next = node->next;

    if (next == NULL) {
        (*head_ptr) = NULL;
        return node;
    }

    while(next->next != NULL) {
        node = node->next;
        next = node->next;
    }

    node->next = NULL;
    return next;
}

void insert_node(lru_node_t ** head_ptr, lru_node_t * node, int index) {
    if ((*head_ptr) == NULL || index == 0) {
        node->next = (*head_ptr);
        (*head_ptr) = node;
        return;
    }

    int curr_index = 1;
    lru_node_t * curr_node = (*head_ptr);

    while (curr_node->next != NULL && curr_index < index) {
        curr_node = curr_node->next;
        curr_index++;
    }

    node->next = curr_node->next;
    curr_node->next = node;
}

lru_node_t * remove_node(lru_node_t ** head_ptr, int64_t flow_id) {
    if (*head_ptr == NULL) {
        return NULL;
    }

    lru_node_t * curr = *head_ptr;
    lru_node_t * next = curr->next;

    if (curr->flow_id == flow_id) {
        (*head_ptr) = next;
        curr->next = NULL;
        return curr;
    }

    while (next != NULL) {
        if (next->flow_id == flow_id) {
            curr->next = next->next;
            next->next = NULL;
            return next;
        }
        curr = next;
        next = next->next;
    }
    
    return NULL;
}

lru_node_t * remove_node_return_index(lru_node_t ** head_ptr, int64_t flow_id, int * index) {
    int idx = 0;

    if (*head_ptr == NULL) {
        (*index) = idx;
        return NULL;
    }

    lru_node_t * curr = *head_ptr;
    lru_node_t * next = curr->next;

    if (curr->flow_id == flow_id) {
        (*head_ptr) = next;
        curr->next = NULL;
        (*index) = idx;
        return curr;
    }

    while (next != NULL) {
        idx++;
        if (next->flow_id == flow_id) {
            curr->next = next->next;
            next->next = NULL;
            (*index = idx);
            return next;
        }
        curr = next;
        next = next->next;
    }
    
    return NULL;
}

void insert_L1_arc_node(arc_sram_t * sram, arc_node_t * node, int index) {
    for (int i = sram->l1_count; i > index; i--) {
        sram->l1_cache[i] = sram->l1_cache[i - 1];
    }
    sram->l1_cache[index] = node;
    sram->l1_count++;
}

void insert_L2_arc_node(arc_sram_t * sram, arc_node_t * node, int index) {
    for (int i = sram->l2_count; i > index; i--) {
        sram->l2_cache[i] = sram->l2_cache[i - 1];
    }
    sram->l2_cache[index] = node;
    sram->l2_count++;
}

arc_node_t * remove_L1_arc_node(arc_sram_t * sram, int index) {
    arc_node_t * node = sram->l1_cache[index];
    for (int i = index; i < sram->l1_count - 1; i++) {
        sram->l1_cache[i] = sram->l1_cache[i + 1];
    }
    sram->l1_cache[sram->l1_count - 1] = NULL;
    sram->l1_count--;
    return node;
}

arc_node_t * remove_L2_arc_node(arc_sram_t * sram, int index) {
    arc_node_t * node = sram->l2_cache[index];
    for (int i = index; i < sram->l2_count - 1; i++) {
        sram->l2_cache[i] = sram->l2_cache[i + 1];
    }
    sram->l2_cache[sram->l2_count - 1] = NULL;
    sram->l2_count--;
    return node;
}

void reinsert_L1_arc_node(arc_sram_t * sram, int index) {
    arc_node_t * node = sram->l1_cache[index];
    for (int i = index; i > 0; i--) {
        sram->l1_cache[i] = sram->l1_cache[i - 1];
    }
    sram->l1_cache[0] = node;
}

void reinsert_L2_arc_node(arc_sram_t * sram, int index) {
    arc_node_t * node = sram->l2_cache[index];
    for (int i = index; i > 0; i--) {
        sram->l2_cache[i] = sram->l2_cache[i - 1];
    }
    sram->l2_cache[0] = node;
}

int push_s3f_node(s3f_queue_t * fifo, s3f_node_t * node) {
    int next;

    // Determine where node will be pushed
    next = fifo->head + 1;
    if (next >= fifo->size) {
        next = 0;
    }

    // Only push node if FIFO is not full
    if (next != fifo->tail) {
        fifo->data[fifo->head] = node;
        fifo->head = next;
        return 0;
    }
    else {
        free(node);
        return -1;
    }
}

s3f_node_t * pop_s3f_node(s3f_queue_t * fifo) {
    int next;

    // Only pop node if FIFO is not empty
    if (fifo->head == fifo->tail) {
        return NULL;
    }

    // Determine where tail will be after pop
    next = fifo->tail + 1;
    if (next >= fifo->size) {
        next = 0;
    }

    s3f_node_t * node = fifo->data[fifo->tail];
    fifo->tail = next;
    return node;
}
    
int push_sve_node(sve_sram_t * sram, sve_node_t * node) {
    sve_node_t ** fifo = sram->fifo;
    int next = sram->head + 1;
    if (next >= sram->capacity) {
        next = 0;
    }

    if (next != sram->tail) {
        fifo[sram->head] = node;
        sram->head = next;
        return 0;
    }
    else {
        free(node);
        return -1;
    }
}

sve_node_t * remove_sve_node(sve_sram_t * sram, int index) {
    sve_node_t * node = sram->fifo[index];
    if (node == NULL) {
        return NULL;
    }

    for (int i = index; ((i + sram->capacity) % sram->capacity) != sram->tail; i--) {
        sram->fifo[((i + sram->capacity) % sram->capacity)] = sram->fifo[((i - 1 + sram->capacity) % sram->capacity)];
    }
    sram->fifo[sram->tail] = NULL;
    sram->tail = (sram->tail + 1) % sram->capacity;
    
    return node;
}

sram_t * create_sram(int32_t size, int16_t initialize) {
    sram_t * sram = malloc(sizeof(sram_t));
    MALLOC_TEST(sram, __LINE__);
    sram->capacity = size;
    sram->count = 0;
    sram->head = NULL;

    if (initialize == 1) {
        initialize_sram(sram);
    }

    return sram;
}

lfu_node_t * create_lfu_node(int64_t flow_id, int64_t val) {
    lfu_node_t * node = malloc(sizeof(lfu_node_t));
    MALLOC_TEST(node, __LINE__);
    node->flow_id = flow_id;
    node->val = val;
    node->frequency = 0;

    return node;
}

lfu_sram_t * create_lfu_sram(int32_t size, int16_t initialize) {
    lfu_sram_t * sram = malloc(sizeof(lfu_sram_t));
    sram->capacity = size;
    sram->count = 0;
    sram->cache = malloc(sizeof(lfu_node_t *) * size);
    
    for (int i = 0; i < size; i++) {
        sram->cache[i] = NULL;
    }

    if (initialize == 1) {
        initialize_lfu_sram(sram);
    }

    return sram;
}

arc_node_t * create_arc_node(int64_t flow_id, int64_t val) {
    arc_node_t * node = malloc(sizeof(arc_node_t));
    MALLOC_TEST(node, __LINE__);
    node->flow_id = flow_id;
    node->val = val;
    node->frequency = 0;

    return node;
}

arc_sram_t * create_arc_sram(int32_t size, int16_t initialize) {
    arc_sram_t * sram = malloc(sizeof(arc_sram_t));
    sram->capacity = size; // size = 2c
    sram->l1_count = 0;
    sram->l2_count = 0;
    sram->l1_cache = malloc(sizeof(arc_sram_t *) * size / 2); // maximum L1 cache size is c
    sram->l2_cache = malloc(sizeof(arc_sram_t *) * size); // maximum L2 cache size is 2c

    for (int i = 0; i < size; i++) {
        if (i < size / 2) {
            sram->l1_cache[i] = NULL;
        }
        sram->l2_cache[i] = NULL;
    }

    if (initialize == 1) {
        initialize_arc_sram(sram);
    }

    return sram;
}

s3f_node_t * create_s3f_node(int64_t flow_id, int64_t val) {
    s3f_node_t * node = malloc(sizeof(s3f_node_t));
    MALLOC_TEST(node, __LINE__);
    node->flow_id = flow_id;
    node->val = val;
    node->frequency = 0;

    return node;
}

s3f_queue_t * create_s3f_queue(int size) {
    s3f_queue_t * fifo = malloc(sizeof(s3f_queue_t));
    fifo->head = 0;
    fifo->tail = 0;
    fifo->size = size;
    fifo->data = malloc(sizeof(s3f_node_t *) * size);

    return fifo;
}

s3f_sram_t * create_s3f_sram(int32_t size, int16_t initialize) {
    s3f_sram_t * sram = malloc(sizeof(s3f_sram_t));
    sram->capacity = size;

    sram->s_fifo = create_s3f_queue(size / 10);
    sram->m_fifo = create_s3f_queue(size - size / 10);
    sram->g_fifo = create_s3f_queue(size - size / 10);

    if (initialize == 1) {
        initialize_s3f_sram(sram);
    }

    return sram;
}

sve_node_t * create_sve_node(int64_t flow_id, int64_t val) {
    sve_node_t * node = malloc(sizeof(sve_node_t));
    MALLOC_TEST(node, __LINE__);
    node->flow_id = flow_id;
    node->val = val;
    node->visited = 0;

    return node;
}

sve_sram_t * create_sve_sram(int32_t size, int16_t initialize) {
    sve_sram_t  * sram = malloc(sizeof(sve_sram_t));
    sram->capacity = size;

    sram->head = 0;
    sram->tail = 0;
    sram->hand = 0;
    sram->fifo = malloc(sizeof(sve_node_t *) * size);

    for (int i = 0; i < size; i++) {
        sram->fifo[i] = NULL;
    }

    if (initialize == 1) {
        initialize_sve_sram(sram);
    }

    return sram;
}
 
dm_sram_t * create_dm_sram(int32_t size, int16_t initialize) {
    dm_sram_t * sram = malloc(sizeof(dm_sram_t));
    MALLOC_TEST(sram, __LINE__);
    sram->capacity = size;
    sram->flow_ids = malloc(sizeof(int64_t) * size);
    sram->memory = malloc(sizeof(int64_t) * size);
    for (int i = 0; i < size; i++) {
        if (initialize == 1) {
            sram->flow_ids[i] = i;
        }
        else {
            if (i == 0) {
                sram->flow_ids[i] = 1;
            }
            else {
                sram->flow_ids[i] = 0;
            }
        }
        sram->memory[i] = 0;
    }

    return sram;
}

dram_t * create_dram(int32_t size, int32_t delay) {
    dram_t * dram = malloc(sizeof(dram_t));
    MALLOC_TEST(dram, __LINE__);
    dram->capacity = size;
    dram->delay = delay;
    dram->accesses = 1;
    dram->memory = malloc(sizeof(int64_t) * size); // Contains value associated with flow_id (i.e. val)
    dram->accessible = malloc(sizeof(int) * size); // When using DRAM-only, shows which values are accessible at any given moment
    for (int i = 0; i < size; i++) {
        dram->memory[i] = 0;
        dram->accessible[i] = 0;
    }

    dram->lock = 0;
    dram->accessing = -1;
    dram->placement_idx = 0;
    
    return dram;
}

void initialize_sram(sram_t * sram) {
    for (int i = 0; i < sram->capacity; i++) {
        lru_node_t * node = create_lru_node(i, 0);
        node->next = sram->head;
        sram->head = node;
    }
    sram->count = sram->capacity;
}

void initialize_lfu_sram(lfu_sram_t * sram) {
    for (int i = 0; i < sram->capacity; i++) {
        lfu_node_t * node = create_lfu_node(i, 0);
        sram->cache[i] = node;
    }
    sram->count = sram->capacity;
}

void initialize_arc_sram(arc_sram_t * sram) {
    for (int i = 0; i < sram->capacity / 2; i++) {
        arc_node_t * node = create_arc_node(i, 0);
        sram->l1_cache[i] = node;
    }
    sram->l1_count = sram->capacity / 2;
}

void initialize_s3f_sram(s3f_sram_t * sram) {
    for (int i = 0; i < sram->capacity; i++) {
        s3f_node_t * node = create_s3f_node(i, 0);
        if (i < sram->capacity / 10) {
            push_s3f_node(sram->s_fifo, node);
        }
        else {
            push_s3f_node(sram->m_fifo, node);
        }
    }
}

void initialize_sve_sram(sve_sram_t * sram) {
    for(int i = 0; i < sram->capacity; i++) {
        sve_node_t * node = create_sve_node(i, 0);
        push_sve_node(sram, node);
    }
}

void initialize_dm_sram(dm_sram_t * sram) {
    for (int i = 0; i < sram->capacity; i++) {
        sram->flow_ids[i] = i;
    }
}

int64_t evict_from_sram(sram_t * sram, dram_t * dram) {
    if (sram->head != NULL) {
        lru_node_t * evicted = pop(&(sram->head));
        sram->count--;
        int64_t flow_id = evicted->flow_id;
        int64_t val = evicted->val;
        dram->memory[flow_id % DRAM_SIZE] = val;
        free(evicted);
        return flow_id;
    }
    else {
        // Nothing to evict
        return -1;
    }
}

int64_t evict_from_lfu_sram(lfu_sram_t * sram, dram_t * dram) {
    if (sram->count > 0) {
        int min_freq = 999999;
        int min_idx = -1;
        for (int i = sram->count - 1; i >= 0; i--) {
            if (sram->cache[i]->frequency < min_freq) {
                min_idx = i;
                min_freq = sram->cache[i]->frequency;
            }

            if (min_freq = 0) {
                break;
            }
        }

        int64_t flow_id = sram->cache[min_idx]->flow_id;
        int64_t val = sram->cache[min_idx]->val;

        free(sram->cache[min_idx]);
        sram->count--;

        for (int i = min_idx; i < sram->count; i++) {
            sram->cache[i] = sram->cache[i + 1];
        }
        sram->cache[sram->count] = NULL;

        dram->memory[flow_id % DRAM_SIZE] = val;

        return flow_id;
    }
    else {
        // Nothing to evict
        return -1;
    }
}

int64_t evict_from_arc_sram(arc_sram_t * sram, dram_t * dram) {
    if (sram->l1_count + sram->l2_count > 0) {
        arc_node_t * evicted = NULL;
        // L1 >= c: evict from L1
        if (sram->l1_count >= sram->capacity / 2) {
            sram->l1_count--;
            evicted = sram->l1_cache[sram->l1_count];
            sram->l1_cache[sram->l1_count] = NULL;
        }
        // L1 < c: evict from L2
        else {
            sram->l2_count--;
            evicted = sram->l2_cache[sram->l2_count];
            sram->l2_cache[sram->l2_count] = NULL;
        }

        int64_t flow_id = evicted->flow_id;
        int64_t val = evicted->val;

        free(evicted);

        dram->memory[flow_id % DRAM_SIZE] = val;

        return flow_id;
    }
    else {
        return -1;
    }
}

int64_t evict_from_s3f_sram_s(s3f_sram_t * sram, dram_t * dram) {
    s3f_queue_t * s_fifo = sram->s_fifo;
    s3f_queue_t * m_fifo = sram->m_fifo;
    s3f_queue_t * g_fifo = sram->g_fifo;

    while (s_fifo->head != s_fifo->tail) {
        s3f_node_t * t = pop_s3f_node(s_fifo);
        if (t == NULL) {
            return -1;
        }
        // Move t from S FIFO to M FIFO
        if (t->frequency > 1) {
            // If M FIFO is full, evict an entry before inserting
            if ((m_fifo->head + 1) % m_fifo->size == m_fifo->tail) {
                evict_from_s3f_sram_m(sram, dram);
            }
            push_s3f_node(m_fifo, t);
        }
        else {
            if ((g_fifo->head + 1) % g_fifo->size == g_fifo->tail) {
                s3f_node_t * ghost_node = pop_s3f_node(g_fifo);
                dram->memory[ghost_node->flow_id % DRAM_SIZE] = ghost_node->val;
                free(ghost_node);
            }
            push_s3f_node(g_fifo, t);
            return t->flow_id; 
        }
    }

    return -1;
}

int64_t evict_from_s3f_sram_m(s3f_sram_t * sram, dram_t * dram) {
    s3f_queue_t * m_fifo = sram->m_fifo;

    while(m_fifo->head != m_fifo->tail) {
        s3f_node_t * t = pop_s3f_node(m_fifo);
        if (t == NULL) {
            return -1;
        }
        if (t->frequency > 0) {
            push_s3f_node(m_fifo, t);
            t->frequency--;
        }
        else {
            int64_t flow_id = t->flow_id;
            dram->memory[flow_id % DRAM_SIZE] = t->val;
            free(t);
            return flow_id;
        }
    }
}

int64_t evict_from_sve_sram(sve_sram_t * sram, dram_t * dram) {
    sve_node_t * node = remove_sve_node(sram, sram->hand);
    if (node == NULL) {
        return -1;
    }
    int64_t flow_id = node->flow_id;
    dram->memory[flow_id % DRAM_SIZE] = node->val;
    free(node);
    return flow_id;
}

int64_t pull_from_dram(sram_t * sram, dram_t * dram, int64_t flow_id) {
    lru_node_t * node = create_lru_node(flow_id, dram->memory[flow_id]);
    insert_node(&(sram->head), node, dram->placement_idx);
    //push(&(sram->head), node);
    sram->count++;
    if (sram->count > sram->capacity) {
        evict_from_sram(sram, dram);
    }
    return node->val;
}

int64_t pull_from_dram_lfu(lfu_sram_t * sram, dram_t * dram, int64_t flow_id) {
    lfu_node_t * node = create_lfu_node(flow_id, dram->memory[flow_id]);
    if (sram->count >= sram->capacity) {
        evict_from_lfu_sram(sram, dram);
    }
    sram->cache[sram->count] = node;
    sram->count++;
    return node->val;
}

int64_t pull_from_dram_arc(arc_sram_t * sram, dram_t * dram, int64_t flow_id) {
    arc_node_t * node = create_arc_node(flow_id, dram->memory[flow_id]);
    node->frequency++;
    if (sram->l1_count > sram->capacity / 2 || sram->l1_count + sram->l2_count >= sram->capacity) {
        evict_from_arc_sram(sram, dram);
    }
    insert_L1_arc_node(sram, node, 0);
    return node->val;
}

int64_t pull_from_dram_s3f(s3f_sram_t * sram, dram_t * dram, int64_t flow_id) {
    s3f_node_t * node = create_s3f_node(flow_id, dram->memory[flow_id]);
    node->frequency++;
    if ((sram->s_fifo->head + 1) % sram->s_fifo->size == sram->s_fifo->tail) {
        evict_from_s3f_sram_s(sram, dram);
    }
    // Determine if flow id is in ghost queue
    int is_in_g_fifo = 0;
    for (int i = sram->g_fifo->tail; i % sram->g_fifo->size < sram->g_fifo->head; i++) {
        if (sram->g_fifo->data[i % sram->g_fifo->size]->flow_id == flow_id) {
            is_in_g_fifo = 1;
            break;
        }
    }
    int push_result = -1;
    if (is_in_g_fifo) {
        push_result = push_s3f_node(sram->m_fifo, node);
    }
    else {
        push_result = push_s3f_node(sram->s_fifo, node);
    }
    if (push_result == -1) {
        return -1;
    }
    return node->val;
}

int64_t pull_from_dram_sve(sve_sram_t * sram, dram_t * dram, int64_t flow_id) {
    // If SRAM is full
    if ((sram->head + 1) % sram->capacity == sram->tail) {
        int curr = sram->hand;
        sve_node_t * node = sram->fifo[curr];
        if (node == NULL) {
            curr = sram->tail;
            node = sram->fifo[curr];
        }
        while (node->visited == 1) {
            node->visited = 0;
            curr++;
            curr %= sram->capacity;
            node = sram->fifo[curr];
            if (node == NULL) {
                curr = sram->tail;
                node = sram->fifo[curr];
            }
        }
        sram->hand = curr;
        evict_from_sve_sram(sram, dram);
        sram->hand++;
        sram->hand %= sram->capacity;
    }

    // Insert x to head of T
    sve_node_t * node = create_sve_node(flow_id, dram->memory[flow_id]);
    push_sve_node(sram, node);

    return node->val;
}

int64_t access_sram(sram_t * sram, int64_t flow_id) {
    lru_node_t * node = remove_node(&(sram->head), flow_id);
    if (node != NULL) {
        // Cache hit
        node->val++;
        push(&(sram->head), node);
        return node->val;
    }
    // Cache miss
    return -1;
}

int64_t access_sram_return_index(sram_t * sram, int64_t flow_id) {
    int idx = 0;
    lru_node_t * node = remove_node_return_index(&(sram->head), flow_id, &idx);
    if (node != NULL) {
        // Cache hit
        node->val++;
        push(&(sram->head), node);
        return idx;
    }
    // Cache miss
    return -1;
}

int64_t access_lfu_sram(lfu_sram_t * sram, int64_t flow_id) {
    for (int i = 0; i < sram->count; i++) {
        if (sram->cache[i]->flow_id == flow_id) {
            sram->cache[i]->frequency++;
            sram->cache[i]->val++;
            return sram->cache[i]->val;
        }
    }
    // Cache miss
    return -1;
}

int64_t access_arc_sram(arc_sram_t * sram, int64_t flow_id) {
    // Access L1 cache
    for (int i = 0; i < sram->l1_count; i++) {
        arc_node_t * node = sram->l1_cache[i];
        // Cache hit
        if (node->flow_id == flow_id) {
            if (node->frequency > 0) {
                // Move to front of L2 cache
                node = remove_L1_arc_node(sram, i);
                insert_L2_arc_node(sram, node, 0);
            }
            else {
                // Move to front of L1 cache
                reinsert_L1_arc_node(sram, i);
            }
            node->frequency++;
            node->val++;
            return node->val;
        }
    }
    // Access L2 cache
    for (int i = 0; i < sram->l2_count; i++) {
        arc_node_t * node = sram->l2_cache[i];
        // Cache hit
        if (node->flow_id == flow_id) {
            // Move to front of L2 cache
            reinsert_L2_arc_node(sram, i);
            node->frequency++;
            node->val++;
            return node->val;
        }
    }
    // Cache miss
    return -1;
}

int64_t access_s3f_sram(s3f_sram_t * sram, int64_t flow_id) {
    // Check S FIFO
    for (int i = sram->s_fifo->tail; i % sram->s_fifo->size != sram->s_fifo->head; i++) {
        // Cache hit
        s3f_node_t * node = sram->s_fifo->data[i % sram->s_fifo->size];
        if (node->flow_id == flow_id) {
            node->frequency++;
            if (node->frequency > 3) {
                node->frequency = 3;
            }
            node->val++;
            return node->val;
        }
    }
    // Check M FIFO
    for (int i = sram->m_fifo->tail; i % sram->m_fifo->size != sram->m_fifo->head; i++) {
        // Cache hit
        s3f_node_t * node = sram->m_fifo->data[i % sram->m_fifo->size];
        if (node->flow_id == flow_id) {
            node->frequency++;
            if (node->frequency > 3) {
                node->frequency = 3;
            }
            node->val++;
            return node->val;
        }
    }
    // Cache miss
    return -1;
}

int64_t access_sve_sram(sve_sram_t * sram, int64_t flow_id) {
    for (int i = sram->tail; (i  % sram->capacity) != sram->head; i++) {
        // Cache hit
        sve_node_t * node = sram->fifo[i % sram->capacity];
        if (node == NULL) {
            return -1;
        }
        if (node->flow_id == flow_id) {
            node->visited = 1;
            node->val++;
            return node->val;
        }
    }
    // Cache miss
    return -1;
}

int64_t evict_from_dm_sram(dm_sram_t * sram, dram_t * dram, int64_t flow_id) {
    int64_t evict_id = sram->flow_ids[flow_id % sram->capacity];
    dram->memory[evict_id] = sram->memory[evict_id % sram->capacity];
    return evict_id;
}

int64_t dm_pull_from_dram(dm_sram_t * sram, dram_t * dram, int64_t flow_id) {
    evict_from_dm_sram(sram, dram, flow_id);
    sram->flow_ids[flow_id % sram->capacity] = flow_id;
    sram->memory[flow_id % sram->capacity] = dram->memory[flow_id];
    return sram->memory[flow_id % sram->capacity];
}

int64_t access_dm_sram(dm_sram_t * sram, int64_t flow_id) {
    if (sram->flow_ids[flow_id % sram->capacity] == flow_id) {
        // Cache hit 
        sram->memory[flow_id % sram->capacity]++;
        return sram->memory[(flow_id % sram->capacity)]; // SUCCESS!!!
    }
    // Cache miss
    return -1; // FAILURE!!!
}

int64_t reorganize_sram(sram_t * sram, buffer_t * buffer) {
    // buffer must be a buffer of int64_t from snapshot
    int k = -1;
    for (int i = buffer->num_elements - 1; i >= 0; i--) {
        int64_t * id = (int64_t *) buffer_peek(buffer, i);
        int result = access_sram(sram, *id);
        if (result < 0) {
            k = *id;
        }
    }
    return k;
}

void print_sram(sram_t * sram) {
    lru_node_t * head = sram->head;
    printf("Current sram state\n");
    while (head != NULL) {
        printf("%d, %d || ", (int) head->flow_id, (int) head->val);
        head = head->next;
    }
    printf("\n");
}

void print_lfu_sram(lfu_sram_t * sram) {
    printf("Current sram state\n");
    for (int i = 0; i < sram->count; i++) {
        printf("%d (%d), %d || ", (int) sram->cache[i]->flow_id, (int) sram->cache[i]->frequency, (int) sram->cache[i]->val);
    }
    printf("\n");
}

void print_arc_sram(arc_sram_t * sram) {
    printf("Current sram state\n");
    printf("L1 cache:\n");
    for (int i = 0; i < sram->l1_count; i++) {
        printf("%d (%d), %d || ", (int) sram->l1_cache[i]->flow_id, (int) sram->l1_cache[i]->frequency, (int) sram->l1_cache[i]->val);
    }
    printf("\nL2 cache:\n");
    for (int i = 0; i < sram->l2_count; i++) {
        printf("%d (%d), %d || ", (int) sram->l2_cache[i]->flow_id, (int) sram->l2_cache[i]->frequency, (int) sram->l2_cache[i]->val);
    }
    printf("\n");
}

void print_s3f_sram(s3f_sram_t * sram) {
    printf("Current sram state\n");
    printf("S FIFO:\n");
    for (int i = sram->s_fifo->tail; i % sram->s_fifo->size != sram->s_fifo->head; i++) {
        printf("%d, %d (%d) || ", (int) sram->s_fifo->data[i % sram->s_fifo->size]->flow_id, (int) sram->s_fifo->data[i % sram->s_fifo->size]->val, (int) sram->s_fifo->data[i % sram->s_fifo->size]->frequency);
    }
    printf("\nM FIFO:\n");
    for (int i = sram->m_fifo->tail; i % sram->m_fifo->size != sram->m_fifo->head; i++) {
        printf("%d, %d || ", (int) sram->m_fifo->data[i % sram->m_fifo->size]->flow_id, (int) sram->m_fifo->data[i % sram->m_fifo->size]->val, (int) sram->m_fifo->data[i % sram->m_fifo->size]->frequency);
    }
    printf("\nG FIFO:\n");
    for (int i = sram->g_fifo->tail; i % sram->g_fifo->size != sram->g_fifo->head; i++) {
        printf("%d || ", (int) sram->g_fifo->data[i % sram->g_fifo->size]->flow_id);
    }
    printf("\n");
}

void print_sve_sram(sve_sram_t * sram) {
    printf("Current sram state\n");
    for (int i = sram->tail; (i % sram->capacity) != sram->head; i++) {
        printf("%d, %d (%d) || ", (int) sram->fifo[i % sram->capacity]->flow_id, (int) sram->fifo[i % sram->capacity]->val, (int) sram->fifo[i % sram->capacity]->visited);
    }
    printf("\n");
}

void print_dm_sram(dm_sram_t * sram) {
    printf("Current sram state\n");
    for (int i = 0; i < sram->capacity; i++) {
        printf("%d, %d ||", (int) sram->flow_ids[i], (int) sram->memory[i]);
    }
}

int64_t belady(sram_t * sram, dram_t * dram, int64_t * lin_queue, int q_len) {
    if (q_len > 0) {
        for (int j = 0; j < q_len; j++) {
            int64_t id = lin_queue[j];
            if (access_sram(sram, id) < 0) {
                lru_node_t * node = create_lru_node(id, dram->memory[id]);
                push(&(sram->head), node);
                sram->count++;
                if (sram->count > sram->capacity) {
                    evict_belady(sram, dram, lin_queue, q_len);
                }
                return 0;
            }
        }
    }
    return -1;
}

int64_t evict_belady(sram_t * sram, dram_t * dram, int64_t * lin_queue, int q_len) {
    if (q_len > 0) {
        // Find flow in SRAM latest to be accessed
        for (int i = q_len - 1; i >= 0; i--) {
            int64_t id = lin_queue[i];
            access_sram(sram, id);
        }
        // Last index will be latest to access
        lru_node_t * evicted = pop(&(sram->head));
        sram->count--;
        int64_t flow_id = evicted->flow_id;
        int64_t val = evicted->val;
        dram->memory[flow_id % DRAM_SIZE] = val;
        free(evicted);
        return flow_id;
    }
    else {
        evict_from_sram(sram, dram);
    }
    return -1;
}

void free_sram(sram_t * sram) {
    lru_node_t * head = sram->head;
    while (head != NULL) {
        //printf("free sram node %d\n", (int) head->flow_id);
        lru_node_t * next = head->next;
        free(head);
        head = next;
    }
    free(sram);
}

void free_lfu_sram(lfu_sram_t * sram) {
    for (int i = 0; i < sram->count; i++) {
        free(sram->cache[i]);
    }
    free(sram->cache);
    free(sram);
}

void free_arc_sram(arc_sram_t * sram) {
    for (int i = 0; i < sram->l1_count; i++) {
        free(sram->l1_cache[i]);
    }
    for (int i = 0; i < sram->l2_count; i++) {
        free(sram->l2_cache[i]);
    }
    free(sram->l1_cache);
    free(sram->l2_cache);
    free(sram);
}

void free_s3f_queue(s3f_queue_t * fifo) {
    if (fifo->size > 0) {
        for (int i = fifo->tail; i % fifo->size != fifo->head; i++) {
            free(fifo->data[i % fifo->size]);
        }
    }
    
    free(fifo->data);
    free(fifo);
}

void free_s3f_sram(s3f_sram_t * sram) {
    free_s3f_queue(sram->s_fifo);
    free_s3f_queue(sram->m_fifo);
    free_s3f_queue(sram->g_fifo);
    free(sram);
}

void free_sve_sram(sve_sram_t * sram) {
    for (int i = sram->tail; i % sram->capacity != sram->head; i++) {
        free(sram->fifo[i % sram->capacity]);
    }

    free(sram->fifo);
    free(sram);
}

void free_dm_sram(dm_sram_t * sram) {
    free(sram->flow_ids);
    free(sram->memory);
    free(sram);
}

void free_dram(dram_t * dram) {
    free(dram->memory);
    free(dram->accessible);
    free(dram);
}
