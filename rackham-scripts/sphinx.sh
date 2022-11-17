#!/bin/bash -l

#SBATCH -A snic2022-22-557
#SBATCH -p node
#SBATCH -N 1
#SBATCH -n 1
#SBATCH -t 04:00:00
#SBATCH -J icar_run_sphinx

module load gcc/10.1.0

echo "sphinx"

BINARY_ROOT=/home/alisemi/icar/spec2006
BINARY_NAME=sphinx

OUTPUT_NAME=${BINARY_NAME}_output
PIN_ROOT=/home/alisemi/graph-frameworks/pin-3.24-98612-g6bd5931f2-gcc-linux/
TRACER_ROOT=/home/alisemi/icar/ICAR2022/pin-memory-tracer

cd ${BINARY_ROOT}
$PIN_ROOT/pin -t $TRACER_ROOT/obj-intel64/memory_tracer.so -o ${OUTPUT_NAME} -- ./${BINARY_NAME} ctlfile . args.an4 3>&1 1>&2 2>&3 | gzip > ${OUTPUT_NAME}_trace.gz

echo "finished!"
