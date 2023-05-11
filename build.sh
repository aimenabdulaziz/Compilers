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
# 2. Parse the MiniC using Lex and Yacc
# 3. Perform semantic analysis on the MiniC file
# 4. Exit if the analysis fails
# 5. Generate LLVM Intermediate Representation (IR) code (using ir_generator.cpp and clang)
# 6. Build the optimization executable with or without debug flag
# 7. Optimize the generated IR code
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
(cd frontend && make $debug_flag)

# build the AST and run semantic Analysis
frontend/miniC.out $1.c

# if the semantic analysis fails, exit
if [ $? -ne 0 ]; then
    exit 1
fi

# create IR code
clang -S -emit-llvm $1.c -o $1.ll 

# Clean and build the optimization executable
(cd optimization && make $debug_flag)

# call the optimizer and pass the miniC file and generated IR code 
optimization/./optimizer $1.ll

# Clean the executables
(cd frontend && make clean)
(cd optimization && make clean)
