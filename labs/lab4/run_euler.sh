#!/bin/bash

#bsub -J "no[1-16]" -N "awk -v jindex=\$LSB_JOBINDEX 'NR==jindex' euler/commands_no | bash"
#bsub -J "next[1-16]" -N "awk -v jindex=\$LSB_JOBINDEX 'NR==jindex' euler/commands_next | bash"
#bsub -J "stride[1-16]" -N "awk -v jindex=\$LSB_JOBINDEX 'NR==jindex' euler/commands_stride | bash"
#bsub -J "ghb[1-16]" -N "awk -v jindex=\$LSB_JOBINDEX 'NR==jindex' euler/commands_ghb | bash"
#bsub -J "fbp[1-16]" -N "awk -v jindex=\$LSB_JOBINDEX 'NR==jindex' euler/commands_fdp | bash"
#bsub -J "markov[1-16]" -N "awk -v jindex=\$LSB_JOBINDEX 'NR==jindex' euler/commands_markov | bash"
bsub -J "pangloss[1-16]" -N "awk -v jindex=\$LSB_JOBINDEX 'NR==jindex' euler/commands_pangloss | bash"
