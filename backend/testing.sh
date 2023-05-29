#!/bin/bash

# MiniC Backend testing script

# usage: ./testing.sh or make test (strongly recommended)
# Compile the codegen executable
make

# Set the directory containing the test files
dir=../tests/backend

# Loop over each test file in the directory
for file in `ls "$dir"/*.c | grep -v main.c`; do
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
    if grep -q "= read()" "$dir/$base.c"; then
        # Generate a random integer between 1 and 100
        input=$(shuf -i 1-100 -n 1)

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
        echo "Test failed: "$base""
        echo "Expected:"
        echo "$expected"
        echo "Got:"
        echo "$output"
    else
        echo "Test passed: "$base""
        rm -f "$dir"/"$base".out "$dir"/"$base".s "$dir"/"$base".ll "$dir"/"$base".expected
    fi
done