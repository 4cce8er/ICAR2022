#!/bin/bash -l

#SBATCH -A p2017001
#SBATCH -p core
#SBATCH -n 1
#SBATCH -t 01:00:00
#SBATCH -J icar_run_microbenchmark
#SBATCH --qos=p2017001_24nodes

BINARY_ROOT=/home/alisemi/icar/ICAR2022/microbenchmark
BINARY_NAME=icar

echo "icar-microbenchmar"

OUTPUT_NAME=${BINARY_NAME}_output
PIN_ROOT=/home/alisemi/graph-frameworks/pin-3.24-98612-g6bd5931f2-gcc-linux/
TRACER_ROOT=/home/alisemi/icar/ICAR2022/pin-memory-tracer

cd ${BINARY_ROOT}
$PIN_ROOT/pin -t $TRACER_ROOT/obj-intel64/memory_tracer.so -o ${OUTPUT_NAME} -- ./${BINARY_NAME} 3>&1 1>&2 2>&3 | gzip > ${OUTPUT_NAME}_trace.gz

echo "finished!"

