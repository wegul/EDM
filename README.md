# EDM: An Ultra-Low Latency Ethernet Fabric for Memory Disaggregation

EDM is a novel network fabric design that achieves ultra-low latency memory disaggregation over Ethernet in datacenter environments. This project was published at ASPLOS 2025.

## Overview
Modern datacenters are moving towards disaggregated architectures where memory resources are separated from compute nodes. However, accessing remote memory over traditional Ethernet networks incurs significant latency overhead. EDM addresses this challenge through two key innovations:

1.	PHY-Layer Network Stack: EDM implements the entire network stack for remote memory access within the Physical layer (PHY) of Ethernet, bypassing the traditional transport overhead.

2.	Centralized Flow Scheduler: A fast, in-network memory flow scheduler operates in the switch’s PHY layer, creating dynamic virtual circuits between compute and memory nodes to eliminate queuing delays.

Performance
- ~300ns end-to-end latency in unloaded networks.
- Maintains latency within 1.3x of unloaded performance even under high network loads.

## Repository Structure

This repository contains two main components:

1.	FPGA Implementation
- Verilog implementation for Xilinx Alveo U200. [EDM-PHY](https://github.com/wegul/EDM-PHY/tree/master)

2. Hardware Simulation
- YCSB workload and hw simulator. [hwsimu](https://github.com/wegul/EDM/tree/main/hwsimu)

3.	Network Simulator
- Trace file generator based on real-world workloads. [tracegen](https://github.com/wegul/EDM-tracegen/tree/master)
- 144-node single rack network simulator. [simulator](https://github.com/wegul/EDM/tree/main/netsimu)

## Getting started
OS: Ubuntu 22.04  
SW: Vivado 2023.2   
HW: Xilinx Alevo U200 FPGA board  
Dependency: Sklearn, Matplotlib, Pandas, Python >= 3.9 
Please do

    pip3 install pandas matplotlib scikit-learn

#### Installation
    git clone https://github.com/wegul/EDM.git
    cd EDM
    git submodule update --init


### A. Hardware verification
Please follow [instructions](https://github.com/wegul/EDM-PHY).



### B. Hardware simulation
This section reproduces Figure-6,7 in artifact evaluation.


#### Build and run

1. Compile
```
    cd EDM/hwsimu
    mkdir -p build
    cd build
    cmake ..
    make
```

2. To generate traces, do

```
    cd ../scripts
    ./gen_trace.sh
```


3. To run the above two experiment and get results, do
```
    ./run_all.sh 
```
4. The final results and figures are in _hwsimu/result_. For convenience, the results of our paper is in _hwsimu/result/golden.result_. Since traces are randomly generated, there might be <10% variation.



#### Dataset - YCSB

We use YCSB dataset as kv-store memory traces to demonstrate EDM's bandwidth efficiency and end-to-end latency. For convenience, YCSB workloads are pre-acquired in _EDM\_simu/ycsb\_raw\_output_. 


#### Bandwidth utilization

In this experiment, we empirically calculated the overhead of inter-packet gap and header encapsulation in EDM and RDMA to infer theoretical bandwidth utilization in real world traces.

#### End-to-end latency
This experiment is based on the latency profile of EDM hardware testbed as well as a local DDR3 module on FPGA, with average access latency of ~82ns. In this experiment, we randomly allocate objects in YCSB traces into local and remote according to their addresses (keys).
Since the distribution of object accesses in YCSB is zipfian, which will affect our final result for end-to-end latency, each raw trace will be shuffled *10* times. Also, error bars are added.





### C. Network simulation
This section reproduces Figure-8 in artifact evaluation.

1. **Build:**  

First, build trace generator. Inside _EDM/EDM-tracegen/_ directory:
```
    cd EDM/EDM-tracegen
    autoconf  
    autoreconf -i  
    ./configure  
    make
```
'autoconf' might throw an error. We can ignore and redo.   
Next, generate traces. This will create  _EDM/netsimu/testdir_
```
    cd ./EDM-workload
    ./generate_traces.sh
```
2. **Compile and run:** 
This will create _EDM/netsimu/results/_
```
    cd EDM/netsimu
    ./compile.sh
    ./run_tests.sh
```
Note:  
  i) Please do next step after all experiments are finished. You can check via `screen -ls`.  
  ii) Our script runs every experiment in parallel and spawns >120 screen sessions. Your system might end up killing some of them and result in missing files. In case that happens, please contact us and we will provide access to our server. 

3. **Collect results and plot:**
```
     ./get_result.sh
```
The generated graphs are in _EDM/netsimu/results/_. Note that for _mixed\_*_ traces, the graph only include three groups because other two pure RREQ and pure WREQ are in _rreq\_result.csv_ and _wreq\_result.csv_, respectively.

For convenience, averaged results of our submission is in _EDM/netsimu/results/golden.result_. Since the flow traces are randomly generated and ordered, there might be <10% difference.





## Cite
```
    @misc{su2024edmultralowlatencyethernet,
        title={EDM: An Ultra-Low Latency Ethernet Fabric for Memory Disaggregation}, 
        author={Weigao Su and Vishal Shrivastav},
        year={2024},
        eprint={2411.08300},
        archivePrefix={arXiv},
        primaryClass={cs.OS},
        url={https://arxiv.org/abs/2411.08300}, 
    }
```