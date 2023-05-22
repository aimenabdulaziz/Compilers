#!/bin/bash
#
# MiniC Optimizations testing script
#
# usage: ./testing.sh or make test (strongly recommended)
#
# Author: Aimen Abdulaziz
# Date: Spring 2023


# List of files to run
dir=../tests/optimization
SRC=./optimizer

# Loop through each file and run it with miniC.out
for i in {1..6}; do
    echo "Running test$i.ll"
    echo
    $SRC "$dir/test$i.ll"
    echo "----------------------------------------"
done
