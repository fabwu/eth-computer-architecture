import subprocess
import argparse

import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

policies = ["FCFS", "FRFCFS", "FRFCFS_Cap", "ATLAS", "BLISS"]

traces = {
        "H": "./traces/high-mem-intensity.trace",
        "L": "./traces/low-mem-intensity.trace",
     }

workloads = ["HLLL", "HHLL", "HHHH", "HHHHHHHH"]

stat_file = "plot.stats"
csv_file = "plot.csv"

def get_stat(stat):
    with open(stat_file) as f:
        for line in f:
            rows = line.split()
            if rows[0] == stat:
                return float(rows[1])

def run_ramulator(policy, traces):
    command = ["./ramulator", "./configs/DDR4-config.cfg",
                              "--mode=cpu",
                              "--stats", stat_file,
                              "--policy", f"{policy}",
                            ]
    command.extend(traces)
    proc = subprocess.run(command, capture_output=True, check=True, text=True)
    print(proc.stdout)

def run_simulation():
    data = []

    for policy in policies:
        run_ramulator(policy, [traces["L"]])
        cpu_cycles_alone = {}
        cpu_cycles_alone["L"] = get_stat("ramulator.cpu_cycles")

        run_ramulator(policy, [traces["H"]])
        cpu_cycles_alone["H"] = get_stat("ramulator.cpu_cycles")

        for workload in workloads:
            trace_files = []
            for c in workload:
                trace_files.append(traces[c])

            run_ramulator(policy, trace_files)
            inst_throughput = get_stat("ramulator.inst_throughput")
            max_slowdown = -1
            for coreid, c in enumerate(workload):
                cpu_cycles_shared = get_stat(f"ramulator.record_cycs_core_{coreid}")
                max_slowdown = max(max_slowdown, cpu_cycles_shared / cpu_cycles_alone[c])

            data.append([policy, workload, round(inst_throughput, 2), round(max_slowdown, 2)])

    df = pd.DataFrame(data, columns=["policy", "workload", "inst_throughput", "max_slowdown"])
    df.to_csv(csv_file, index=False)
    return df

def plot(df, name, label):
    sns.set_theme()
    sns.set_style("whitegrid")
    sns.set_context("paper")
    g = sns.catplot(x="workload", y=name, hue="policy", data=df, kind="bar")
    g.set_axis_labels("", label)
    g.legend.set_title(None)
    #g.fig.set_size_inches(4.7747, 3.5)
    plt.savefig(f"report/img/{name}.pdf", format="pdf")
    #plt.show()

def main():
    parser = argparse.ArgumentParser(description='Simulate and plot scheduling policies.')
    parser.add_argument('--sim', const=True, default=False, help="Run simulation", nargs="?")

    args = parser.parse_args()
    do_simulation = args.sim

    try:
        df  = pd.read_csv(csv_file)
    except FileNotFoundError:
        do_simulation = True

    if do_simulation:
        df = run_simulation()

    plot(df, "inst_throughput", "Instruction Throughput")
    plot(df, "max_slowdown", "Max. Slowdown")

if __name__ == "__main__":
    main()
