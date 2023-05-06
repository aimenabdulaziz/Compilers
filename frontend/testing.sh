# MiniC Compiler testing script
#
# usage: ./testing.sh or make test (strongly recommended)
#
# Author: Aimen Abdulaziz
# Date: Spring, 2023
#
#!/bin/bash

# List of files to run
dir=tests

# Loop through each file and run it with miniC.out
for i in {1..7}; do
    echo "Running lex_yacc_$dir/p$i.c"
    echo
    ./miniC_main.out "lex_yacc_$dir/p$i.c"
    echo "----------------------------------------"
done

# Loop through each file and run it with miniC.out
for i in {1..4}; do
    echo "Running semantic_$dir/p$i.c"
    echo
    ./miniC_main.out "semantic_$dir/p$i.c"
    echo "----------------------------------------"
done