#include "numa.h"

int num_of_flow = 0, num_of_finished_flow = 0;
float latency = 0;
std::queue<Flow *> flowlist;
// host send
std::priority_queue<Flow *, std::vector<Flow *>, cmpFlow> wq;       // work queue
std::priority_queue<Flow *, std::vector<Flow *>, cmpFlow> ctl2dram; // ctrl send command to dram
// std::priority_queue<Flow *, std::vector<Flow *>, cmpFlow> dram2fst; // from dram to data buffer&FST
std::priority_queue<Flow *, std::vector<Flow *>, cmpFlow> ctl2net;
// std::priority_queue<Flow *, std::vector<Flow *>, cmpFlow> net2xcv;
// switch
std::priority_queue<Flow *, std::vector<Flow *>, cmpFlow> net2sw; // PD
std::priority_queue<Flow *, std::vector<Flow *>, cmpFlow> sw2net;
// host recv
// std::priority_queue<Flow *, std::vector<Flow *>, cmpFlow> xcv2net;
std::priority_queue<Flow *, std::vector<Flow *>, cmpFlow> net2ctl; // update flow state table
std::priority_queue<Flow *, std::vector<Flow *>, cmpFlow> sink;
char *inputFileName = NULL, *outputFileName = NULL;

int readTrace(FILE *fp)
{
    // FILE *fp = fopen(fileName.c_str(), "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        return 1;
    }
    int flowID;
    char type[8], addr[8];
    while (fscanf(fp, "%d, %8[^,],%s", &flowID, &type[0], &addr[0]) >= 2)
    {
        Flow *flow = (Flow *)malloc(sizeof(Flow));
        flow->flowID = flowID;
        flow->flowType = strcmp(type, "READ") == 0 ? TYPE_RD : TYPE_WR;
        flow->targetAddr = strcmp(addr, "LOCAL") == 0 ? ADDR_LC : ADDR_RM;
        flow->time2deq = 0;
        flow->startTime = -1;
        flowlist.push(flow);
        num_of_flow++;
    }
    fclose(fp);
}
int writeResult(Flow *flow, int current_time, FILE *fp)
{
    // FILE *fp = fopen("out.csv", "w");
    if (fp == NULL)
    {
        perror("Error opening file");
        return 1;
    }
    int flowID, start;
    std::string type, addr;
    flowID = flow->flowID;
    type = flow->flowType == TYPE_WR ? "WRITE" : "READ";
    addr = flow->targetAddr == ADDR_LC ? "LOCAL" : "REMOTE";
    start = flow->startTime;
    fprintf(fp, "%d,%s,%s,%d,%d\n", flowID, type.c_str(), addr.c_str(), start, current_time);
}

void simulation(FILE *fp_in, FILE *fp_out)
{
    fprintf(fp_out, "ID,TYPE,ADDR,START,END\n");
    readTrace(fp_in);

    int current_time = 0;
    int stop = 0;
    while (!stop)
    {
        /*
            Host send
        */
        // 1. Read from WQ
        Flow *wqe = wq.empty() ? NULL : new Flow(*wq.top());
        if (wqe && wqe->time2deq <= current_time)
        {
            wq.pop();
            // Read from wq to get addr in DRAM
            wqe->time2deq = current_time + RD_BUF + PRTCL;
            if (wqe->flowType != TYPE_RD) // Needs to extract from DRAM (WR or RS)
            {
                wqe->time2deq += RD_DRAM + 2 * RD_BUF + UPD_FST;
            }
            ctl2net.push(wqe);
        }
        // 2. Read from flowlist
        else
        {
            wqe = flowlist.empty() ? NULL : new Flow(*flowlist.front());
            if (wqe && wqe->time2deq <= current_time)
            {
                flowlist.pop();
                wqe->startTime = current_time;
                if (wqe->targetAddr == ADDR_LC) // it is a local operation
                {
                    wqe->time2deq = current_time + (wqe->flowType == TYPE_RD ? RD_DRAM : WR_DRAM);
                    sink.push(wqe);
                }
                else // Remote
                {
                    wqe->time2deq = current_time + 2 * RD_BUF;
                    wq.push(wqe);
                }
            }
        }

        /*
            Net_module send to SW
        */
        Flow *net_send_flow = ctl2net.empty() ? NULL : new Flow(*ctl2net.top());
        if (net_send_flow && net_send_flow->time2deq <= current_time)
        {
            ctl2net.pop();
            net_send_flow->time2deq = current_time + NET_DELAY + XCVR + PD;
            net2sw.push(net_send_flow);
        }

        /*
            SW send+recv
        */
        Flow *sw_flow = net2sw.empty() ? NULL : new Flow(*net2sw.top());
        if (sw_flow && sw_flow->time2deq <= current_time)
        {
            net2sw.pop();
            sw_flow->time2deq = current_time + SW_FWD + 2 * XCVR + PD;
            if (sw_flow->flowType == TYPE_RD)
            {
                sw_flow->time2deq += SW_FWD + WR_DRAM;
            }
            // if (sw_flow->flowType == TYPE_WR)
            // {
            //     sw_flow->time2deq -= RD_DRAM + 2 * RD_BUF + WR_DRAM;
            // }

            sw2net.push(sw_flow);
        }

        /*
            Net recv
        */
        Flow *net_recv_flow = sw2net.empty() ? NULL : new Flow(*sw2net.top());
        if (net_recv_flow && net_recv_flow->time2deq <= current_time)
        {
            sw2net.pop();
            net_recv_flow->time2deq = current_time + NET_DELAY + XCVR;
            // if (net_recv_flow->flowType == TYPE_WR)
            // {
            //     net_recv_flow->time2deq *= 2;
            // }

            net2ctl.push(net_recv_flow);
        }

        /*
            Host recv
        */
        // controller update
        Flow *host_recv_flow = net2ctl.empty() ? NULL : new Flow(*net2ctl.top());
        if (host_recv_flow && host_recv_flow->time2deq <= current_time)
        {
            net2ctl.pop();
            if (host_recv_flow->flowType == TYPE_RD) // Recv a RREQ
            {
                host_recv_flow->flowType = TYPE_RS;
                host_recv_flow->time2deq = current_time + 2 * RD_BUF + UPD_FST + RD_DRAM; // recv addr, put in grantQ and update FST
                sink.push(host_recv_flow);
            }
            else if (host_recv_flow->flowType == TYPE_WR || host_recv_flow->flowType == TYPE_RS)
            {
                /* put in DRAM */
                host_recv_flow->time2deq = current_time + WR_DRAM + PRTCL + 2 * RD_BUF + UPD_FST;
                sink.push(host_recv_flow);
            }
        }

        /*
            DRAM fetch according to command & transfer result to FST
            ctl2dram has only one source which is when Rx gets a RREQ and needs to fetch RRES
        */
        Flow *dram_cmd = ctl2dram.empty() ? NULL : new Flow(*ctl2dram.top());
        if (dram_cmd && dram_cmd->time2deq <= current_time)
        {
            ctl2dram.pop();
            assert("ctl2dram type should be only RRES" && dram_cmd->flowType == TYPE_RS);
            dram_cmd->time2deq = current_time + RD_DRAM + UPD_FST;
            wq.push(dram_cmd);
        }

        /*
            DRAM write, flow sink
        */
        Flow *dram_wr_flow = sink.empty() ? NULL : new Flow(*sink.top());
        if (dram_wr_flow && dram_wr_flow->time2deq <= current_time)
        {
            sink.pop();
            // Finished flow
            writeResult(dram_wr_flow, current_time, fp_out);
            latency += current_time - dram_wr_flow->startTime;
            num_of_finished_flow++;
            if (num_of_finished_flow >= num_of_flow)
            {
                stop = 1;
                fclose(fp_out);
            }
        }
        current_time += 1;
    }

    latency /= num_of_finished_flow;
    printf("%s,%.3f\n", inputFileName, latency * cycle_in_ns);
}

char *getCmdOption(char **begin, char **end, const std::string &option)
{
    char **itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(char **begin, char **end, const std::string &option)
{
    return std::find(begin, end, option) != end;
}

int main(int argc, char *argv[])
{
    // char *inputFileName = "testin.csv", *outputFileName = "testout.csv";

    if (!cmdOptionExists(argv, argv + argc, "-fi") || !cmdOptionExists(argv, argv + argc, "-fo"))
    {
        perror("Need -fi, -fo option.\n");
        return -1;
    }
    else
    {
        inputFileName = getCmdOption(argv, argv + argc, "-fi");
        outputFileName = getCmdOption(argv, argv + argc, "-fo");
    }
    FILE *fp_in = fopen(inputFileName, "r"), *fp_out = fopen(outputFileName, "w");
    simulation(fp_in, fp_out);

    return 0;
}
