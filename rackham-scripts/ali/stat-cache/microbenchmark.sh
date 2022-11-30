#!/bin/bash -l

#SBATCH -A p2017001
#SBATCH -p node
#SBATCH -N 1
#SBATCH -n 1
#SBATCH -t 04:00:00
#SBATCH	-C mem256GB
#SBATCH -J icar_run_lbm
#SBATCH --qos=p2017001_24nodes

echo "microbenchmark"

BINARY_ROOT=/home/alisemi/icar/ICAR2022/microbenchmark
BINARY_NAME=icar

OUTPUT_NAME=${BINARY_NAME}_output_trace.gz
TRACER_ROOT=/home/alisemi/icar/ICAR2022/cache_simulator/statCache

echo "lbm 100"
cd ${BINARY_ROOT}
${TRACER_ROOT} ./${OUTPUT_NAME} 0

echo "finished!"
