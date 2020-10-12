#!/usr/bin/python

import sys, os, subprocess, re, glob, argparse, configparser
import matplotlib
import matplotlib.pyplot as plt
import numpy as np

sim = "./sim"

bold="\033[1m"
green="\033[0;32m"
red="\033[0;31m"
normal="\033[0m"

matplotlib.use("pgf")
matplotlib.rcParams.update({
    "pgf.texsystem": "pdflatex",
    'font.family': 'serif',
    'text.usetex': True,
    'pgf.rcfonts': False,
})

def main():
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1)  
    fig.set_size_inches(w=4.7747, h=5)        
    
    plot_cache_size(ax1)
    plot_block_size(ax2)
    plot_num_ways(ax3)
    plt.savefig('report/stream.pgf')

def plot_cache_size(ax):
    size_list, ipc_list = benchmark_stream_cache_size()
    size_with_unit = [f"{x}KB" for x in size_list]
    baseline_ipc = ipc_list[0]
    ipc_normalized = [ipc / baseline_ipc for ipc in ipc_list]
    ax.bar(size_with_unit, ipc_normalized)

def plot_block_size(ax):
    size_list, ipc_list = benchmark_stream_block_size()
    size_with_unit = [f"{x}" for x in size_list]
    baseline_ipc = ipc_list[0]
    ipc_normalized = [ipc / baseline_ipc for ipc in ipc_list]
    ax.bar(size_with_unit, ipc_normalized)

def plot_num_ways(ax):
    num_ways_list, ipc_list = benchmark_stream_num_ways()
    num_ways_with_unit = [f"{x}" for x in num_ways_list]
    baseline_ipc = ipc_list[0]
    ipc_normalized = [ipc / baseline_ipc for ipc in ipc_list]
    ax.bar(num_ways_with_unit, ipc_normalized)

def benchmark_stream_cache_size():
    config = {
        "DEFAULT": {
            "Input": "inputs/benchmark/stream_64K.x"
        },
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

    total_size_list = [0, 1, 2, 4, 8, 16, 32, 64, 128, 256]
    ipc_list = []
    for total_size in total_size_list:
        print(bold + "Benchmark: " + normal + f"Stream with cache size {total_size}")
        config["DATA CACHE"]["TotalSize"] = total_size * 1024
        sim_out = benchmark(config)
        if not sim_out:
            print(red + "ERROR -- no output from simulator" + normal)
            continue
        stats = {}
        for line in sim_out.split("\n"):
            x = line.split(": ");
            stats[x[0]] = x[1]

        ipc_list.append(float(stats['IPC']))

        print("Cycles: " + stats['Cycles'])
        print("IPC: " + stats['IPC'])
        print("Data Hit/Miss: " + stats['DataHit'] + "/" + stats['DataMiss'])
        print("Inst Hit/Miss: " + stats['InstHit'] + "/" + stats['InstMiss'])
    
    return total_size_list, ipc_list

def benchmark_stream_block_size():
    config = {
        "DEFAULT": {
            "Input": "inputs/benchmark/stream_64K.x"
        },
        "INSTRUCTION CACHE": {
            "TotalSize": 8 * 1024,
            "BlockSize": 32,
            "NumWay": 3
        },
        "DATA CACHE": {
            "TotalSize": 32 * 1024,
            "NumWay": 1
        }
    }

    block_size_list = [4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048]
    ipc_list = []
    for block_size in block_size_list:
        print(bold + "Benchmark: " + normal + f"Stream with block size {block_size}")
        config["DATA CACHE"]["BlockSize"] = block_size
        sim_out = benchmark(config)
        if not sim_out:
            print(red + "ERROR -- no output from simulator" + normal)
            continue
        stats = {}
        for line in sim_out.split("\n"):
            x = line.split(": ");
            stats[x[0]] = x[1]

        ipc_list.append(float(stats['IPC']))

        print("Cycles: " + stats['Cycles'])
        print("IPC: " + stats['IPC'])
        print("Data Hit/Miss: " + stats['DataHit'] + "/" + stats['DataMiss'])
        print("Inst Hit/Miss: " + stats['InstHit'] + "/" + stats['InstMiss'])
        print()

    return block_size_list, ipc_list

def benchmark_stream_num_ways():
    config = {
        "DEFAULT": {
            "Input": "inputs/benchmark/stream_64K.x"
        },
        "INSTRUCTION CACHE": {
            "TotalSize": 8 * 1024,
            "BlockSize": 32,
            "NumWay": 3
        },
        "DATA CACHE": {
            "TotalSize": 32 * 1024,
            "BlockSize": 32
        }
    }

    num_ways_list = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512]
    ipc_list = []
    for num_ways in num_ways_list:
        print(bold + "Benchmark: " + normal + f"Stream with {num_ways} ways")
        config["DATA CACHE"]["NumWay"] = num_ways
        sim_out = benchmark(config)
        if not sim_out:
            print(red + "ERROR -- no output from simulator" + normal)
            continue
        stats = {}
        for line in sim_out.split("\n"):
            x = line.split(": ");
            stats[x[0]] = x[1]

        ipc_list.append(float(stats['IPC']))

        print("Cycles: " + stats['Cycles'])
        print("IPC: " + stats['IPC'])
        print("Data Hit/Miss: " + stats['DataHit'] + "/" + stats['DataMiss'])
        print("Inst Hit/Miss: " + stats['InstHit'] + "/" + stats['InstMiss'])
        print()

    return num_ways_list, ipc_list
 
def run(i):
    global ref, sim

    refproc = subprocess.Popen([ref, i], executable=ref, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    simproc = subprocess.Popen([sim, i], executable=sim, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    cmds = b""
    cmdfile = os.path.splitext(i)[0] + ".cmd"
    if os.path.exists(cmdfile):
      cmds += open(cmdfile).read().encode('utf-8')

    cmds += b"\ngo\nrdump\nquit\n"
    (r, r_err) = refproc.communicate(input=cmds)
    (s, s_err) = simproc.communicate(input=cmds)

    return filter_stats(r.decode('utf-8')), filter_stats(s.decode('utf-8'))

def benchmark(config):
    global sim

    i = config["DEFAULT"]["Input"]
    simproc = subprocess.Popen(
            [sim, i], 
            executable=sim, 
            stdin=subprocess.PIPE, 
            stdout=subprocess.PIPE, 
            stderr=subprocess.PIPE,
            env = {
                "INST_CACHE_TOTAL_SIZE": str(config["INSTRUCTION CACHE"]["TotalSize"]),
                "INST_CACHE_BLOCK_SIZE": str(config["INSTRUCTION CACHE"]["BlockSize"]),
                "INST_CACHE_NUM_WAY": str(config["INSTRUCTION CACHE"]["NumWay"]),
                "DATA_CACHE_TOTAL_SIZE": str(config["DATA CACHE"]["TotalSize"]),
                "DATA_CACHE_BLOCK_SIZE": str(config["DATA CACHE"]["BlockSize"]),
                "DATA_CACHE_NUM_WAY": str(config["DATA CACHE"]["NumWay"])
            }
        )

    cmds = b""
    cmdfile = os.path.splitext(i)[0] + ".cmd"
    if os.path.exists(cmdfile):
      cmds += open(cmdfile).read().encode('utf-8')

    cmds += b"\ngo\nrdump\nquit\n"
    (s, s_err) = simproc.communicate(input=cmds)

    return filter_stats(s.decode('utf-8'))

def filter_stats(out):
    lines = out.split("\n")
    regex = re.compile("^(HI:)|(LO:)|(R\d+:)|(PC:)|(Cycles:)|(Fetched\w+:)|(Retired\w+:)|(IPC:)|(Flushes:)|(Data\w+:)|(Inst\w+:).*$")
    out = []
    for l in lines:
        if regex.match(l):
          out.append(l)

    return "\n".join(out)


if __name__ == "__main__":
    main()

