import os, subprocess, re

sim = "./sim"

# console colors
bold="\033[1m"
green="\033[0;32m"
red="\033[0;31m"
normal="\033[0m"

benchmarks = ["stream", "strided", "locality", "random"]

def get_input_file(benchmark):
    return f"inputs/benchmark/{benchmark}_64K.x"

def run(config):
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
                "INST_CACHE_POLICY": str(config["INSTRUCTION CACHE"]["ReplacementPolicy"]),

                "DATA_CACHE_TOTAL_SIZE": str(config["DATA CACHE"]["TotalSize"]),
                "DATA_CACHE_BLOCK_SIZE": str(config["DATA CACHE"]["BlockSize"]),
                "DATA_CACHE_NUM_WAY": str(config["DATA CACHE"]["NumWay"]),
                "DATA_CACHE_POLICY": str(config["DATA CACHE"]["ReplacementPolicy"])
            }
        )

    cmds = b""
    cmdfile = os.path.splitext(i)[0] + ".cmd"
    if os.path.exists(cmdfile):
      cmds += open(cmdfile).read().encode('utf-8')

    cmds += b"\ngo\nrdump\nquit\n"
    (s, s_err) = simproc.communicate(input=cmds)
    #print(s_err)
    return filter_stats(s.decode('utf-8'))

def filter_stats(out):
    lines = out.split("\n")
    regex = re.compile("^(HI:)|(LO:)|(R\d+:)|(PC:)|(Cycles:)|(Fetched\w+:)|(Retired\w+:)|(IPC:)|(Flushes:)|(Data\w+:)|(Inst\w+:).*$")
    out = []
    for l in lines:
        if regex.match(l):
          out.append(l)

    return "\n".join(out)
