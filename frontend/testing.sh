#!/bin/bash
#
# MiniC Frontend testing script
#
# usage: ./testing.sh or make test (strongly recommended)
#
# Author: Aimen Abdulaziz
# Date: Spring 2023

# Makefile for compiling the code
make

# Define the list of files to run
# dir: directory containing the test files
# SRC: name of the frontend source file
# lex: name of the subdirectory containing the lex/yacc test files
# semantic: name of the subdirectory containing the semantic analysis test files
dir=../tests
SRC=frontend
lex=lex_yacc
semantic=semantic_analysis

# Define ANSI escape codes for colors
# RED: red color for terminal output (not currently used)
# GREEN: green color for terminal output
# NC: reset color to default (No Color)
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' 

# Loop through each lex/yacc test file and run it with frontend
# Each file in the $dir/$lex directory with .c extension is run through the frontend
# The return value of the frontend operation is checked
# If the return value is non-zero for p6.c and p7.c, the test is considered passed because
# we expect these test to fail.
# If the return value is zero for other files, the test is considered passed
# Otherwise, the test is considered failed
for file in `ls "$dir"/"$lex"/*.c`; do
    echo "Running $file"
    ./frontend "$file"
    base=`basename "$file" .c`
    ret_value=$?
    if [[ $ret_value -ne 0 && ( "$file" == *"/p6.c" || "$file" == *"/p7.c" ) ]]; then
        echo -e "${GREEN}Test passed: $file${NC}"
    elif [ $ret_value -eq 0 ]; then
        echo -e "${GREEN}Test passed: $file${NC}"
    else
        echo -e "${RED}Test failed: $file${NC}"
    fi
    rm -f "$dir"/"$lex"/"$base"_manual.ll
    echo "----------------------------------------"
done

# Loop through each semantic analysis test file and run it with frontend
# Each file in the $dir/$semantic directory with .c extension is run through the frontend
# The return value of the frontend operation is checked
# If the return value is zero, the test is considered failed because all the test files
# in the semantic analysis folder should be successfully parsed but they should fail
# semantic analysis check.
for file in `ls "$dir"/"$semantic"/*.c`; do
    echo "Running $file"
    ./frontend "$file"
    if [ $? -eq 0 ]; then
        echo -e "${RED}Test failed: $file${NC}"
    else
        echo -e "${GREEN}Test passed: $file${NC}"
    fi
    echo "----------------------------------------"
done
