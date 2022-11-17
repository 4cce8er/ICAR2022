#!/bin/bash -l

echo "Output File Basename = $1"
echo "Application = $2"

$PIN_ROOT/pin -t obj-intel64/memory_tracer.so -o $1 -- $2 3>&1 1>&2 2>&3 | gzip > $1_trace.gz

