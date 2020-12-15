#!/bin/bash

if [ "$#" -lt 1 ]; then
    echo "Illegal number of parameters"
    echo "Usage: ./run_euler.sh [BINARY]"
    exit 1
fi

TRACE_LIST=$PWD/scripts/lab_traces.txt
TRACE_DIR=$PWD/traces/
BINARY=${1}
N_WARM=100
N_SIM=500

# Sanity check
#if [ -z $TRACE_DIR ] || [ ! -d "$TRACE_DIR" ] ; then
#    echo "[ERROR] Cannot find a trace directory: $TRACE_DIR"
#    exit 1
#fi
#
#if [ ! -f "$BINARY" ] ; then
#    echo "[ERROR] Cannot find a ChampSim binary: $BINARY"
#    exit 1
#fi

re='^[0-9]+$'
if ! [[ $N_WARM =~ $re ]] || [ -z $N_WARM ] ; then
    echo "[ERROR]: Number of warmup instructions is NOT a number" >&2;
    exit 1
fi

re='^[0-9]+$'
if ! [[ $N_SIM =~ $re ]] || [ -z $N_SIM ] ; then
    echo "[ERROR]: Number of simulation instructions is NOT a number" >&2;
    exit 1
fi

#if [ ! -f "$TRACE_DIR/$TRACE" ] ; then
#    echo "[ERROR] Cannot find a trace file: $TRACE_DIR/$TRACE"
#    exit 1
#fi

while read line; do
    TRACE=${TRACE_DIR}$line

    mkdir -p results_${N_SIM}M
    bsub -R "rusage[mem=4096]" "${BINARY} -warmup_instructions ${N_WARM}000000 -simulation_instructions ${N_SIM}000000 -traces ${TRACE}"
done < ${TRACE_LIST}

