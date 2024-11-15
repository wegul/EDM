#!/bin/bash

mkdir -p "../trace"
mkdir -p "../result"

for ratio in 0.1 0.5 1 2 10; do
    echo "python3 py/gen_runnable.py -n $ratio"
    python3 py/gen_runnable.py -n $ratio
done
