#!/bin/bash -l

#SBATCH -A p2017001
#SBATCH -p node
#SBATCH -N 1
#SBATCH -n 1
#SBATCH -t 04:00:00
#SBATCH	-C mem256GB
#SBATCH -J icar_run_lbm
#SBATCH --qos=p2017001_24nodes

echo "lbm"

BINARY_ROOT=/home/alisemi/icar/spec2006
BINARY_NAME=lbm

OUTPUT_NAME=${BINARY_NAME}_output
PIN_ROOT=/home/alisemi/icar/pin-3.25-98650-g8f6168173-gcc-linux/
TRACER_ROOT=/home/alisemi/icar/ICAR2022/pin-cache-tracer

echo "lbm 100"
cd ${BINARY_ROOT}
$PIN_ROOT/pin -t $TRACER_ROOT/obj-intel64/memory_tracer.so -- ./${BINARY_NAME} 100 reference.dat 0 0 100_100_130_ldc.of

echo "finished!"
