#!/bin/bash
#
# build.sh - A script to build, analyze, and optimize a MiniC file
# 
# Usage: ./build.sh <filename> [DEBUG]
# 
# <filename> - The base name of the MiniC source file (excluding the .c extension)
# [DEBUG]    - (Optional) If present, the script will be run in debug mode
#
# This script performs the following tasks:
# 1. Build the frontend executable with or without debug flag
# 2. Perform semantic analysis on the MiniC file
# 3. Exit if the analysis fails
# 4. Generate LLVM Intermediate Representation (IR) code
# 5. Build the optimization executable with or without debug flag
# 6. Optimize the generated IR code
#
# Author: Aimen Abdulaziz
# Date: Spring, 2023

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
