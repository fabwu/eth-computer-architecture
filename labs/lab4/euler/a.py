#!/usr/bin/env python3

import sys
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

files = [
        "euler/result_no.txt",
#        "euler/dpc3/result_pangloss.txt",
#        "euler/dpc3/result_ipcp.txt",
#        "euler/dpc3/result_sangam.txt",
#        "euler/dpc3/result_berti.txt",
#        "euler/dpc3/result_team_12.txt",
#        "euler/result_next.txt",
#        "euler/result_stride.txt",
        "euler/result_ghb.txt",
        "euler/result_fdb.txt",
        "euler/result_markov.txt",
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
        prefetcher = filename.split(".")[0]
        prefetcher = prefetcher.split("_")[1]
        row.append(prefetcher)
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
        prefetcher = prefetcher.split(".")[0]
        prefetcher = prefetcher.split("_")[1]

        product = 1.0
        num_el = 0
        for line in data:
            if prefetcher == line[0]:
                product *= line[2]
                num_el += 1
        mean_ipc = pow(product, 1/num_el)
        data.append([prefetcher, "Avg.", mean_ipc])
        print(prefetcher + ": " + str(mean_ipc))

    no_lines = [l.copy() for l in data if l[0] == "no"]
    for no_l in no_lines:
        for line in data:
            if line[1] == no_l[1]:
                line[2] = line[2] / no_l[2]

    df = pd.DataFrame(data, columns=["prefetcher",
                                     "trace",
                                     "ipc",
                                     "pre_requested",
                                     "pre_issued",
                                     "pre_useful",
                                     "pre_useless",
                                     "pre_late",
                                     ])
    print(df)
    sns.set_style("whitegrid")
    sns.set_context("paper")
    fig, ax = plt.subplots(figsize=(16, 6))
    g = sns.barplot(x="trace", y="ipc", hue="prefetcher", data=df, ax=ax)

    ax.set_xlabel("")
    ax.set_ylabel("IPC (normalized)")
    plt.tight_layout(pad=0.4)
    plt.legend(loc="upper left", title="Prefetcher")
    plt.savefig(f"report/img/plot.pdf", format="pdf")
    #plt.show()

if __name__ == "__main__":
    main()
