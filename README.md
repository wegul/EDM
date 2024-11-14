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

2.	Network Simulator
- Trace file generator based on real-world workloads. [tracegen](https://github.com/wegul/EDM-tracegen/tree/master)
- 144-node single rack network simulator. [simulator](https://github.com/wegul/EDM/tree/main/simulation)

## Getting started
OS: Ubuntu 22.04  
SW: Vivado 2023.2   
HW: Xilinx Alevo U200 FPGA board  
Dependency: Python >= 3.9
#### Installation
    $ git clone https://github.com/wegul/EDM.git
    $ git submodule update --init


#### Hardware verification
Please follow [instructions](https://github.com/wegul/EDM-PHY).

#### Network simulation

1. **Build:**  

First, build trace generator. Inside _EDM/EDM-tracegen/_ directory:
```
    $ cd EDM/EDM-tracegen
    $ autoconf  
    $ autoreconf -i  
    $ ./configure  
    $ make
```
Next, generate traces. This will create  _EDM/simulation/testdir_
```
    $ cd ./EDM-workload
    $ ./generate_traces.sh
```
2. **Compile and run simulation:** 
This will create _EDM/simulation/results/_
```
    $ cd EDM/simulation
    $ ./compile.sh
    $ ./run_tests.sh
```
3. **Collect results and plot:**
```
    $ python3 parse_log.py
    $ python3 genplot.py
```
The generated graphs are in _EDM/simulation/results/_.



## Citation
TBD
