#!/bin/bash -l

#SBATCH -A p2017001
#SBATCH -p node
#SBATCH -N 1
#SBATCH -n 1
#SBATCH -t 04:00:00
#SBATCH	-C mem256GB
#SBATCH -J icar_run_sphinx
#SBATCH --qos=p2017001_24nodes


echo "sphinx"

BINARY_ROOT=/home/alisemi/icar/spec2006
BINARY_NAME=sphinx

OUTPUT_NAME=${BINARY_NAME}_output
PIN_ROOT=/home/alisemi/icar/pin-3.25-98650-g8f6168173-gcc-linux/
TRACER_ROOT=/home/alisemi/icar/ICAR2022/pin-cache-tracer

cd ${BINARY_ROOT}
$PIN_ROOT/pin -t $TRACER_ROOT/obj-intel64/memory_tracer.so -- ./${BINARY_NAME} ctlfile . args.an4

echo "finished!"
