#!/usr/bin/env python3

import sys

def printIf(line, prefix):
    if line.startswith(prefix):
        print(line)

def main(filename):
    runs = []
    with open(filename) as file:
        run = []
        for line in file:
            line = line.strip()
            run.append(line)

            if line == 'ChampSim Multicore Out-of-Order Simulator':
                runs.append(run)
                run = []

    for run in runs:
        for line in run:
            printIf(line, 'trace_0')
            printIf(line, 'Core_0_L2C_prefetch_requested')
            printIf(line, 'Core_0_L2C_prefetch_issued')
            printIf(line, 'Core_0_L2C_prefetch_useful')
            printIf(line, 'Core_0_L2C_prefetch_useless')
            printIf(line, 'Core_0_L2C_prefetch_late')

        print()

if __name__ == "__main__":
    main(sys.argv[1])
