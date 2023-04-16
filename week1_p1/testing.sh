# MiniC Compiler testing script
#
# Author: Aimen Abdulaziz
# Date: Spring, 2023
#
# usage: ./testing.sh or make test (strongly recommended)
#
#!/bin/bash

# List of files to run
dir=tests

# Loop through each file and run it with miniC.out
for i in {1..7}; do
    ./miniC.out "$dir/p$i.c"
    echo "----------------------------------------"
done