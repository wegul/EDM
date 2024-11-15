#include "edm.h"
#define EDM 0
#define CXL 1
#define RDMA 2

int num_of_flow = 0, num_of_finished_flow = 0;
float latency = 0;
std::queue<Flow*> flowlist;
std::priority_queue<Flow*, std::vector<Flow*>, cmpFlow> fst;
struct Utilz
{
    int sys;
    float activeN;
    float payloadN;
    Utilz(int sysname) {
        sys = sysname;
        activeN = 0;
    };
};

char* inputFileName = NULL, * outputFileName = NULL;

int readTrace(FILE* fp) {
    // FILE *fp = fopen(fileName.c_str(), "r");
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }
    int flowID;
    char type[64], addr[64];
    while (fscanf(fp, "%d, %8[^,],%s", &flowID, &type[0], &addr[0]) >= 2) {
        Flow* flow = (Flow*)malloc(sizeof(Flow));
        flow->flowID = flowID;
        flow->flowType = strcmp(type, "READ") == 0 ? TYPE_RD : TYPE_WR;
        // printf("%s,%d, %d\n", type,strcmp(type, "READ"),flow->flowType==TYPE_RD);
        flow->targetAddr = strcmp(addr, "LOCAL") == 0 ? ADDR_LC : ADDR_RM;
        flow->time2deq = 0;
        flow->startTime = -1;
        flowlist.push(flow);
        num_of_flow++;
    }
    fclose(fp);
}
void simulation(FILE* fp_in) {
    readTrace(fp_in);

    Utilz* edm = new Utilz(EDM);
    Utilz* rdma = new Utilz(RDMA);

    // line rate=103.125
    Flow* flow = flowlist.front();
    float reqs = 0; // in per 8Bytes
    int w = 0;
    while (!flowlist.empty()) {
        flowlist.pop();

        if (flow->flowType == TYPE_RD) {
            // RREQ , payload = 8B addr
            reqs += 1;
            edm->activeN += 8 / 8 + 1;
            rdma->activeN += 8 + 1.5; // IPG=1

            // RRES, p = 1000 + 8B addr
            // payload += 64 / 8;
            // edm->activeN += 64 / 8 + 8 / 8 + 1;
            // rdma->activeN += 64 / 8 + 10 + 1.5; // RDMA header = 78Bytes~10#

            // ACK
            // rdma->activeN += 64 / 8; // rdma ack=62bytes
        }
        else {
            // WREQ, p = 1024 + 8B addr
            reqs += 1;
            w += 1;
            edm->activeN += 100 / 8 + 8 / 8 + 1;
            edm->activeN += 1;                   // Notification
            rdma->activeN += 100 / 8 + 10 + 1.5; // RDMA header = 78Bytes~10#
            // ACK
            // rdma->activeN += 64 / 8; // rdma ack=62bytes
        }

        flow = flowlist.front();
    }
    printf("%s, %.3f, %.3f\n", inputFileName, w / (edm->activeN * 2.56 * 1e-3), w / (rdma->activeN * 2.56 * 1e-3));
}
char* getCmdOption(char** begin, char** end, const std::string& option) {
    char** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

int main(int argc, char* argv[]) {
    if (!cmdOptionExists(argv, argv + argc, "-fi")) {
        perror("Need -fi option.\n");
        return -1;
    }
    else {
        inputFileName = getCmdOption(argv, argv + argc, "-fi");
    }
    FILE* fp_in = fopen(inputFileName, "r");
    simulation(fp_in);

    return 0;
}