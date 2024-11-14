#include "packet.h"

packet_t create_packet(int16_t src_node, int16_t dst_node, int64_t flow_id, int64_t size,
                       int64_t seq_num, int64_t pkt_id)
{
    packet_t self = (packet_t)malloc(sizeof(struct packet));
    MALLOC_TEST(self, __LINE__);
    self->pktType = NET_TYPE; // Default Net packet
    self->reqLen = -1;        // Default
    self->batchNum = -1;
    self->remain_size = 0;
    for (int i = 0; i < MAXBATCH; i++)
    {
        self->batchDst[i] = -1;
        self->batchSrc[i] = -1;
        self->batchFlowID[i] = -1;
        self->batchReqLen[i] = -1;
    }

    self->src_node = src_node;
    self->dst_node = dst_node;
    self->flow_id = flow_id;
    self->size = size;
    self->seq_num = seq_num;
    self->ack_num = 0;
    self->pkt_id = pkt_id;
    self->control_flag = 0;
    self->ecn_flag = 0;

    return self;
}

packet_t ack_packet(packet_t pkt, int64_t ack_num)
{
    packet_t ack = (packet_t)malloc(sizeof(struct packet));
    MALLOC_TEST(ack, __LINE__);
    ack->src_node = pkt->dst_node;
    ack->dst_node = pkt->src_node;
    ack->flow_id = pkt->flow_id;
    ack->size = 0;
    ack->seq_num = pkt->seq_num;
    ack->ack_num = ack_num;
    ack->pkt_id = pkt->pkt_id;
    ack->control_flag = 1;
    ack->ecn_flag = pkt->ecn_flag;
    return ack;
}

void print_packet(packet_t self)
{
    printf("pkt: src %d dst %d flow %d seq %d memType %2x, time: %d\n", (int)self->src_node, (int)self->dst_node, (int)self->flow_id, (int)self->seq_num, (int)self->pktType, curr_timeslot);
}

void free_packet(packet_t self)
{
    if (self != NULL)
        free(self);
}
