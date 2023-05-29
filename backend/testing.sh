#!/bin/bash
#######################################################################
#
# This script is designed to test backend programs. It iterates over all 
# .c test files in a specified directory, excluding those named 'main.c'.
# For each of these files, it generates LLVM IR code using Clang and then
# x86 assembly code using a custom 'codegen' executable. It compiles the
# original C file and the generated assembly code into two separate
# executables. 
#
# If the C file requires input (i.e., it contains a call to 'read()'),
# it generates a random integer between 1 and 100 and uses this as input
# for both executables. If the C file does not require input, it simply
# runs both executables without input.
#
# Finally, it compares the output of both executables. If the outputs
# match, it indicates that the test has passed. If the outputs do not
# match, it indicates that the test has failed and displays both the
# expected and actual output.
#
# Colors are used in the terminal output to make the pass/fail results
# more visually distinct. Specifically, green is used for 'passed' messages,
# and red is used for 'failed' messages.
#
# usage: ./testing.sh (make sure ./testing.sh is executable) or use make test
#
# Author: Aimen Abdulaziz
# Date: Spring 2023

# Compile the codegen executable
make

# Set the directory containing the test files
dir=../tests/backend

# Define ANSI escape codes for colors
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Loop over each test file in the directory
for file in `ls "$dir"/*.c | grep -v main.c`; do
    # Exit if main.c is not found
    if [ ! -f "$dir"/main.c ]; then
        echo "main.c not found"
        exit 1
    fi

    echo "Testing $file"

    # Extract the base name of the file without the .c extension
    base=$(basename "$file" .c)

    # Generate LLVM IR code from the C file
    clang -S -emit-llvm "$dir"/"$base".c -o "$dir"/"$base".ll

    # Generate x86 assembly code from the LLVM IR code using the codegen executable
    ./codegen "$dir"/"$base".ll

    # Check if the codegen executable ran successfully
    if [ $? -ne 0 ]; then
        echo "Codegen couldn't run "$dir"/"$base".ll"
        continue
    fi 

    # Compile the C file and the generated assembly code into an executable
    clang "$dir"/main.c "$dir"/"$base".c -o "$dir"/"$base".expected
    clang "$dir"/main.c "$dir"/"$base".s -m32 -o "$dir"/"$base".out

    # Check if the test file requires input
    if grep -q "= *read *(" "$dir/$base.c"; then
        # Generate a random integer between 1 and 100
        input=$(shuf -i 1-1000 -n 1)

        echo "Test "$base" with input "$input""

        # Pass the random integer as input to the expected executable and capture the output
        expected=$(echo "$input" | "./$dir/$base.expected")

        # Run the executable with the random integer as input and capture the output
        output=$(echo "$input" | "./$dir/$base.out")
    else
        # Read the expected output from a file
        expected=$("./$dir/$base.expected")

        # Run the executable and capture the output
        output=$("./$dir/$base.out")
    fi


    # Compare the output to the expected output
    if [ "$output" != "$expected" ]; then
        echo -e "${RED}Test failed: $base${NC}"
        echo -e "${RED}Expected:${NC}"
        echo "$expected"
        echo -e "${RED}Got:${NC}"
        echo "$output"
    else
        echo -e "${GREEN}Test passed: $base${NC}"
    fi

    rm -f "$dir"/"$base".ll "$dir"/"$base".s "$dir"/"$base".expected "$dir"/"$base".out
done