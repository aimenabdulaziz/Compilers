#!/bin/bash

# check the number of arguments 
if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "Usage: ./build.sh <filename> [DEBUG]"
    exit 1
fi

if [ "$2" == "DEBUG" ]; then
    debug_flag="DEBUG=1"
else
    debug_flag=""
fi

# Clean and build the frontend executable
(cd frontend && make clean && make $debug_flag)

# build the AST and run semantic Analysis
frontend/miniC_main.out $1.c

# if the semantic analysis fails, exit
if [ $? -ne 0 ]; then
    exit 1
fi

# create IR code
clang -S -emit-llvm $1.c -o $1.ll 

# Clean and build the optimization executable
(cd optimization && make clean && make $debug_flag)

# call the optimizer and pass the miniC file and generated IR code 
optimization/./optimizer $1.ll
