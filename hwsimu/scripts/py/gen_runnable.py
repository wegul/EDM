import numpy as np
import pandas as pd
import os
import re
import sys
from sklearn.utils import shuffle

localSet = set()
remoteSet = set()


def divide_load(fileName):
    global local_to_remote, localSet, remoteSet
    df = pd.read_csv(fileName, header=None, index_col=0, dtype=str)

    # shuffle
    shuffled = shuffle(df, random_state=None)
    # Reset the index
    shuffled.reset_index(drop=True, inplace=True)

    arr = shuffled.to_numpy(dtype=str)
    num_of_flows = shuffled.shape[0]
    num_of_local = local_to_remote*num_of_flows/(local_to_remote+1)
    num_of_remote = num_of_flows-num_of_local
    for i in range(0, num_of_flows):
        addr = int(arr[i, 1])
        # Assign address based on ratio
        if i <= num_of_remote:
            remoteSet.add(addr)
        else:
            localSet.add(addr)
    # df.to_csv(fileName+"-temp", header=None)


def gen_run(fileName, outName):
    global localSet, remoteSet
    df = pd.read_csv(fileName, header=None, index_col=0, dtype=str)
    for i in range(0, df.shape[0]):
        addr = int(df.iloc[i, 1])
        if addr in remoteSet:
            df.iloc[i, 1] = "REMOTE"
        elif addr in localSet:
            df.iloc[i, 1] = "LOCAL"
        else:
            print(addr)
            raise Exception("one wildcard addr!")
    df.to_csv(outName, header=None)
    localSet.clear()
    remoteSet.clear()


if __name__ == "__main__":
    if sys.argv[1] != "-n" or len(sys.argv) < 3:
        raise Exception("Usage: sep_remote.py -n [local2remote]")
    local_to_remote = float(sys.argv[2])
    # assign directory
    directory = '../db/'

    # iterate over files in
    # that directory
    for filename in os.listdir(directory):
        f = os.path.join(directory, filename)
        # checking if it is a file
        if os.path.isfile(f):
            regex = r'load_[A-Z]\.csv$'
            if re.match(regex, filename):
                for i in range(5):
                    divide_load(f)  # f== ./db/load_X.csv
                    # print(filename)
                    run_csv = directory+"/run_"+filename.split('_')[1]
                    gen_run(run_csv, "../trace/runnable_" +
                            str(local_to_remote)+"_"+"t"+str(i)+"_"+filename)
                    print(run_csv)
