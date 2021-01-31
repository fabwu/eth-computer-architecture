#!/usr/bin/python

import argparse, os
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import simulator as sim

csv_file = "plot_replacment.csv"
policy_list = [
        "lru_lru",
        "lru_mru",
        "mru_mru",
        "mru_lru",
        "fifo",
        "lifo"
]

config = {
    "DEFAULT": {},
    "INSTRUCTION CACHE": {
        "TotalSize": 8 * 1024,
        "BlockSize": 32,
        "NumWay": 4,
        "ReplacementPolicy": "lru_mru",
    },
    "DATA CACHE": {
        "TotalSize": 64 * 1024,
        "BlockSize": 4,
        "NumWay": 1
    }
}

def plot(df, save):
    sns.set_theme()
    sns.set_style("whitegrid")
    sns.set_context("paper")
    g = sns.catplot(x="policy", y="ipc", hue="benchmark", data=df, kind="bar")
    g.set_axis_labels("Policy", "IPC")
    g.legend.remove()
    g.fig.set_figheight(3)
    plt.tight_layout()
    plt.legend(loc="lower right")

    if save:
        plt.savefig(os.path.realpath(__file__ + "/../../img/replacement.pdf"), format="pdf")
    else:
        plt.show()

def simulate():
    data = []
    for benchmark in sim.benchmarks:
        config["DEFAULT"]["Input"] = sim.get_input_file(benchmark)

        for policy in policy_list:
            print(sim.bold + "Benchmark: " + sim.normal + f"{benchmark} with replacment policy {policy}")
            config["DATA CACHE"]["ReplacementPolicy"] = policy
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

            data.append([benchmark, policy, ipc])

    df = pd.DataFrame(data, columns=["benchmark", "policy", "ipc"])
    df.to_csv(csv_file, index=False)
    return df

def main():
    parser = argparse.ArgumentParser(description='Simulate and plot various replacement policies.')
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
