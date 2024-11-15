#!/bin/bash

workspace="./"
edm="../build/edm"
numa="../build/numa"
rdma="../build/rdma"

rm -r ../result
mkdir -p ../result

rm -r ${workspace}/logs
mkdir -p ${workspace}/logs

for filename in ../trace/*; do
    # echo $filename
    regex="runnable_([0-9]+\.[0-9]+)_t[0-9]+_load_([A])"
    if [[ "$filename" =~ $regex ]]; then
        number="${BASH_REMATCH[1]}"
        letter="${BASH_REMATCH[2]}"
        echo "$edm -fi $filename -fo tmp.csv >> ./logs/stat_edm-${letter}-${number}.csv"
        $edm -fi $filename -fo tmp.csv >>./logs/stat_edm-${letter}-${number}.csv
        echo "$numa -fi $filename -fo tmp.csv >> ./logs/stat_numa-${letter}-${number}.csv"
        $numa -fi $filename -fo tmp.csv >>./logs/stat_numa-${letter}-${number}.csv
        echo "$rdma -fi $filename -fo tmp.csv >> ./logs/stat_rdma-${letter}-${number}.csv"
        $rdma -fi $filename -fo tmp.csv >>./logs/stat_rdma-${letter}-${number}.csv
    fi
done
echo "python3 ./py/cal_avgminmax.py"
python3 ./py/cal_avgminmax.py
echo "python3 ./py/plot_latency.py"
python3 ./py/plot_latency.py

# Run goodput exp
goodp="../build/goodput"

for filename in ../trace/*; do
    regex="../trace/runnable_1.0_t1_load_[ABF].csv"
    if [[ "$filename" =~ $regex ]]; then
        echo "$goodp -fi $filename >> ../result/goodp.csv"
        $goodp -fi $filename >>../result/goodp.csv
    fi
done

# plot graph

echo "python3 ./py/plot_goodp.py"
python3 ./py/plot_goodp.py
