#!/usr/bin/env python3

import sys
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

files = [
        "euler/result_no.txt",
        "euler/dpc3/result_pangloss.txt",
        "euler/dpc3/result_ipcp.txt",
        "euler/dpc3/result_sangam.txt",
        "euler/dpc3/result_berti.txt",
        "euler/dpc3/result_team_12.txt",
        "euler/result_next.txt",
        "euler/result_stride.txt",
        "euler/result_ghb.txt",
        "euler/result_fdb.txt",
        ]

def process_file(filename,data):
    runs = []
    with open(filename) as file:
        run = []
        for line in file:
            line = line.strip()
            if line.startswith("Heartbeat CPU"):
                continue

            if line.startswith("trace_0"):
                runs.append(run)
                run = []

            run.append(line)
        runs.append(run)

    for run in runs:
        row = []
        row.append(filename)
        for line in run:
            if len(line.split()) != 2:
                continue

            key, value = line.split()
            if key == 'trace_0':
                #print(filename + ": "+ line)
                trace = value.split(".")[1]
                trace = trace.split("-")[0]
                trace = trace.split("_")[0]
                row.append(trace)

            if key == 'Core_0_IPC':
                row.append(float(value))

            if key in ['Core_0_L2C_prefetch_requested',
                       'Core_0_L2C_prefetch_issued',
                       'Core_0_L2C_prefetch_useful',
                       'Core_0_L2C_prefetch_useless',
                       'Core_0_L2C_prefetch_late',
                       ]:
                row.append(int(value))

        if len(row) > 1:
            data.append(row)

def main():
    data = []
    for filename in files:
        process_file(filename, data);

    for prefetcher in files:
        product = 1.0
        num_el = 0
        for line in data:
            if prefetcher == line[0]:
                product *= line[2]
                num_el += 1
        print(prefetcher + ": " + str(pow(product, 1/num_el)))

    df = pd.DataFrame(data, columns=["prefetcher",
                                     "trace",
                                     "ipc",
                                     "pre_requested",
                                     "pre_issued",
                                     "pre_useful",
                                     "pre_useless",
                                     "pre_late",
                                     ])
    #print(df)
    sns.catplot(x="trace", y="ipc", hue="prefetcher", data=df, kind="bar")
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
