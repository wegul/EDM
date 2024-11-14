from pathlib import Path
import pandas as pd
from collections import defaultdict
import re

# Get all .log files
directory = Path('./testdir')
ccList = ["edm", "ird", "pfabric", "pfc", "dctcp", "cxl", "fastpass"]
traceList = ["hadoop", "spark", "sparksql", "graphlab", "memcached"]
expList = [r"wonly*.log", r"ronly*.log", r"mix*.log", r"proced*.log"]

for pattern in expList:
    log_files = directory.glob(pattern)
    data = defaultdict(list)
    for log_file in log_files:
        fileName = str(log_file).split('/')[-1]
        ccName = fileName.split('_')[-1][:-4]
        expName = fileName.split('_')[1]
        if expName.startswith("shortflow"):
            expName = fileName.split('_')[0]+fileName.split('_')[1][-7:-4]
        # print(ccName,expName)
        with log_file.open() as f:
            lines = f.readlines()[-20:-1]
            for line in lines:
                if line.find("Avg slowdown") >= 0:
                    sld = line.split(' ')[-1]
                    data[ccName].append(float(sld.strip()))
    for key in data:
        data[key].sort()
    if pattern.startswith("proced"):
        df = pd.DataFrame(
            data, index=traceList, columns=ccList)
    else:
        df = pd.DataFrame(data, columns=ccList)
    if pattern.startswith("proced"):
        resultName = "longflow"
    elif pattern.startswith("mix"):
        resultName = "mixshort"
    elif pattern.startswith("ronly"):
        resultName = "rreq"
    else:
        resultName = "wreq"
    df.to_csv("results/"+resultName+"_result.csv")
    print(str(pattern))
    print(df)
