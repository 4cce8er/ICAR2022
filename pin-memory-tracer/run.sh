#!/bin/bash -l

echo "Output Baseline = $1"

$PIN_ROOT/pin -t obj-intel64/memory_tracer.so -o $1 -- ../microbenchmark/a.out | gzip > $1_trace.gz


