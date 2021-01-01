#!/usr/bin/python

import argparse, os
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import simulator as sim

csv_file = "plot_cache_size.csv"
total_size_list = [0, 1, 2, 4, 8, 16, 32, 64, 128, 256]

config = {
    "DEFAULT": {},
    "INSTRUCTION CACHE": {
        "TotalSize": 8 * 1024,
        "BlockSize": 32,
        "NumWay": 3
    },
    "DATA CACHE": {
        "BlockSize": 4,
        "NumWay": 1
    }
}

def plot(df, save):
    sns.set_theme()
    sns.set_style("whitegrid")
    sns.set_context("paper")
    g = sns.catplot(x="size", y="ipc", hue="benchmark", data=df, kind="bar")
    g.set_axis_labels("Cache Size", "IPC")
    g.legend.set_title(None)
    if save:
        plt.savefig(os.path.realpath(__file__ + "/../../img/cache_size.pdf"), format="pdf")
    else:
        plt.show()

def simulate():
    data = []
    for benchmark in sim.benchmarks:
        config["DEFAULT"]["Input"] = sim.get_input_file(benchmark)

        for total_size in total_size_list:
            print(sim.bold + "Benchmark: " + sim.normal + f"{benchmark} with cache size {total_size}")
            config["DATA CACHE"]["TotalSize"] = total_size * 1024
            sim_out = sim.run(config)
            if not sim_out:
                print(sim.red + "ERROR -- no output from simulator" + sim.normal)
                continue
            stats = {}
            for line in sim_out.split("\n"):
                x = line.split(": ");
                stats[x[0]] = x[1]

            ipc = float(stats['IPC'])

            print("Cycles: " + stats['Cycles'])
            print("IPC: " + str(ipc))
            print("Data Hit/Miss: " + stats['DataHit'] + "/" + stats['DataMiss'])
            print("Inst Hit/Miss: " + stats['InstHit'] + "/" + stats['InstMiss'])

            if total_size == 0:
                # baseline
                baseline_ipc = ipc
                data.append([benchmark, total_size, 1.0])
            else:
                data.append([benchmark, total_size, ipc / baseline_ipc])

    df = pd.DataFrame(data, columns=["benchmark", "size", "ipc"])
    df.to_csv(csv_file, index=False)
    return df

def main():
    parser = argparse.ArgumentParser(description='Simulate and plot various cache sizes.')
    parser.add_argument('--sim', const=True, default=False, help="Run simulation", nargs="?")
    parser.add_argument('--save', const=True, default=False, help="Save plots as pdf", nargs="?")

    args = parser.parse_args()
    do_simulation = args.sim
    do_save = args.save

    try:
        df  = pd.read_csv(csv_file)
    except FileNotFoundError:
        do_simulation = True

    if do_simulation:
        df = simulate()

    plot(df, do_save)

if __name__ == "__main__":
    main()
