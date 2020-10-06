#!/usr/bin/python

# Chris Fallin, 2012
# Yoongu Kim, 2013
# Juan Gomez Luna, 2017
# Minesh Patel, 2020

import sys, os, subprocess, re, glob, argparse, configparser

ref = "./basesim"
sim = "./sim"

bold="\033[1m"
green="\033[0;32m"
red="\033[0;31m"
normal="\033[0m"


def main():
    all_test_inputs = glob.glob("inputs/*/*.x")
    all_benchmark_inputs = glob.glob("benchmarks/*/*.ini")

    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=["benchmark", "test"])
    parser.add_argument("inputs", nargs="*", default="")
    parser = parser.parse_args()

    if parser.mode == "benchmark":
        if not parser.inputs:
            parser.inputs = all_benchmark_inputs
        for i in parser.inputs:
            config = configparser.ConfigParser()
            config.read(i)
            if not config.sections():
                print(red + "ERROR -- input file (*.ini) not found: " + i + normal)
                continue

            print(bold + "Benchmark: " + normal + i)
            sim_out = benchmark(config)
            if not sim_out:
                print(red + "ERROR -- no output from simulator" + normal)
                continue
            stats = {}
            for line in sim_out.split("\n"):
                x = line.split(": ");
                stats[x[0]] = x[1]
            print("Cycles: " + stats['Cycles'])
            print("IPC: " + stats['IPC'])
            print("Data Hit/Miss: " + stats['DataHit'] + "/" + stats['DataMiss'])
            print("Inst Hit/Miss: " + stats['InstHit'] + "/" + stats['InstMiss'])

    elif parser.mode == "test":
        if not parser.inputs:
            parser.inputs = all_test_inputs
        for i in parser.inputs:
            if not os.path.exists(i):
                print(red + "ERROR -- input file (*.x) not found: " + i + normal)
                continue

            print(bold + "Testing: " + normal + i)
            ref_out, sim_out = run(i)

            print("  " + "Stats".ljust(14) + "BaselineSim".center(14) + "YourSim".center(14))

            ref_out = ref_out.split("\n")
            sim_out = sim_out.split("\n")
            
            nocheck = 0
            error = 0
            for r, s in zip(ref_out, sim_out):

                r0 = r.split()[0]
                r1 = r.split()[1]
                s1 = s.split()[1]

                print("  " + r0.ljust(14) + r1.center(14) + s1.center(14),)

                if (r0 == "Cycles:"):
                    nocheck = 1
                if (r1 != s1 and nocheck == 0):
                    print("  " + red + "ERROR" + normal)
                    error = 1
            else:
                print()

            if error == 0:
                print("  " + green + "REGISTER CONTENTS OK" + normal)
            else:
                raise ValueError("REGISTER CONTENTS ERROR")
            print()

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
                "INST_CACHE_TOTAL_SIZE": config["INSTRUCTION CACHE"]["TotalSize"],
                "INST_CACHE_BLOCK_SIZE": config["INSTRUCTION CACHE"]["BlockSize"],
                "INST_CACHE_NUM_WAY": config["INSTRUCTION CACHE"]["NumWay"],
                "DATA_CACHE_TOTAL_SIZE": config["DATA CACHE"]["TotalSize"],
                "DATA_CACHE_BLOCK_SIZE": config["DATA CACHE"]["BlockSize"],
                "DATA_CACHE_NUM_WAY": config["DATA CACHE"]["NumWay"]
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

