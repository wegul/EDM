import numpy as np
import pandas as pd
import os
import re


def parse_stat(statsfile):
    df = pd.read_csv(statsfile, header=None)
    newDF = np.zeros(df.shape, dtype=str)
    length = df.shape[0]
    fileName = df.iloc[:, 0]
    latency = df.iloc[:, 1]
    for item in fileName:
        if item.find("A") >= 0 and item.find()


if __name__ == "__main__":
    directory = '/home/weigao/Downloads/ycsb-0.17.0/EDM_simu/scripts/logs'
    for filename in os.listdir(directory):
        f = os.path.join(directory, filename)
        if os.path.isfile(f):
            regex = r'stat_.*\.csv$'
            if re.match(regex, filename):
                parse_stat(f)
