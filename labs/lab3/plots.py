import subprocess
# for each scheduling policy
#   simulate L/H and collect cpu_cycles_alone
#   for each workload
#       run simulation and record IT and MS
#       record: scheduling_policy workload IT MS

policies = ["FCFS"]

traces = {
        "H": ["./traces/high-mem-intensity.trace"],
        "L": ["./traces/low-mem-intensity.trace"],
        "HLLL": [ "./traces/high-mem-intensity.trace", "./traces/low-mem-intensity.trace",
                  "./traces/low-mem-intensity.trace", "./traces/low-mem-intensity.trace"],
        "HHLL": [ "./traces/high-mem-intensity.trace", "./traces/low-mem-intensity.trace",
                  "./traces/low-mem-intensity.trace", "./traces/low-mem-intensity.trace"],
        "HHHH": [ "./traces/high-mem-intensity.trace", "./traces/high-mem-intensity.trace",
                  "./traces/high-mem-intensity.trace", "./traces/high-mem-intensity.trace"],
        "HHHHHHHH": [ "./traces/high-mem-intensity.trace", "./traces/high-mem-intensity.trace",
                  "./traces/high-mem-intensity.trace", "./traces/high-mem-intensity.trace",
                  "./traces/high-mem-intensity.trace", "./traces/high-mem-intensity.trace",
                  "./traces/high-mem-intensity.trace", "./traces/high-mem-intensity.trace"],
     }

workloads = ["HLLL"]

stat_file = "plot.stats"

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

def main():
    data = []
    for policy in policies:
        run_ramulator(policy, traces["L"])
        cpu_cycles_alone_L = get_stat("ramulator.cpu_cycles")

        run_ramulator(policy, traces["H"])
        cpu_cycles_alone_H = get_stat("ramulator.cpu_cycles")

        for workload in workloads:
            run_ramulator(policy, traces[workload])
            inst_throughput = get_stat("ramulator.inst_throughput")
            cpu_cycles_shared = get_stat("ramulator.cpu_cycles")
            max_slowdown_L = cpu_cycles_shared / cpu_cycles_alone_L
            max_slowdown_H = cpu_cycles_shared / cpu_cycles_alone_H
            max_slowdown = max(max_slowdown_L, max_slowdown_H)

            data.append([policy, workload, round(inst_throughput, 2), round(max_slowdown, 2)])

    print(data)

if __name__ == "__main__":
    main()
