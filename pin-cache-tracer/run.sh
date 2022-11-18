#!/bin/bash -l

echo "Application = $1"

$PIN_ROOT/pin -t obj-intel64/memory_tracer.so -- $1

