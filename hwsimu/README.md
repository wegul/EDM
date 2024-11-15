## YCSB workload

We use YCSB dataset as kv-store memory traces to demonstrate EDM's bandwidth efficiency and end-to-end latency. For convenience, YCSB workloads are pre-acquired in _EDM\_simu/ycsb\_raw\_output_. We then allocate objects in YCSB traces into local and remote according to their addresses (keys).
This is done by `gen_runnable.py`, which stores the generated files in _EDM\_simu/scripts/trace_. Since the distribution of object accesses in YCSB is zipfian, which influences our final result for end-to-end latency, each raw trace will be shuffled *10* times.

To generate traces, do

    cd EDM/hwsimu/scripts
    ./gen_trace.sh


## Bandwidth utilization

In this experiment, we empirically calculated the overhead of inter-packet gap and header encapsulation in EDM and RDMA to infer theoretical bandwidth utilization in real world traces.

## Bandwidth utilization
This experiment is based on the latency profile of EDM hardware testbed as well as a local DDR3 module on FPGA, with average access latency of ~82ns.



## Build and run

    cd EDM/hwsimu
    mkdir -p build
    cd build
    cmake ..
    make


To run the above two experiment and get results, do

    ./run_all.sh 

The final results and figures are in _hwsimu/result_.

