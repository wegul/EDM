#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <queue>
#include <vector>
#include <string>
#include <string.h>
#include <fstream>
#include <iostream>
#include <cassert>

#define MAX_FLOW_ID 1024
#define TYPE_RD 0 // Flow RREQ type
#define TYPE_WR 1 // Flow WREQ type
#define TYPE_RS 2
#define ADDR_LC 0 // local addr
#define ADDR_RM 1
//====Meta data====
#define XCVR 8
#define PD 5
//========= Action set and cost (DONT TOUCH!) =======
#define RD_BUF 1
#define SYNC_FIFO 4
#define UPD_FST 7
#define NET_DELAY 2 + 3 // PCS+MAC
#define SW_FWD 2 * NET_DELAY + 157
#define PRTCL 90
//-----DRAM-----
#define RD_DRAM 30
#define WR_DRAM 30
//=====================================

const float cycle_in_ns = 2.56;

struct Host
{
    int hostID; // 0,1
};

struct Flow
{
    int flowID;
    u_int8_t flowType;   // 0READ, 1Write
    u_int8_t targetAddr; // 0Local, 1Remote
    int time2deq;
    int startTime;

    Flow(const Flow &src)
    {
        flowID = src.flowID;
        flowType = src.flowType;
        targetAddr = src.targetAddr;
        time2deq = src.time2deq;
        startTime = src.startTime;
    };
};
struct cmpFlow
{
    bool operator()(Flow *&a, Flow *&b)
    {
        if (a->time2deq == b->time2deq)
        {
            return a->startTime > b->startTime;
        }

        return a->time2deq > b->time2deq;
    }
};