#!/bin/bash

./ramulator ./configs/DDR4-config.cfg --mode=cpu --stats $1 --policy BLISS \
./traces/high-mem-intensity.trace ./traces/high-mem-intensity.trace \
./traces/high-mem-intensity.trace ./traces/low-mem-intensity.trace
