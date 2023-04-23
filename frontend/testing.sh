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
    echo "Running lex_yacc_$dir/p$i.c"
    echo
    ./miniC.out "lex_yacc_$dir/p$i.c"
    echo "----------------------------------------"
done

# Loop through each file and run it with miniC.out
for i in {1..4}; do
    echo "Running semantic_$dir/p$i.c"
    echo
    ./miniC.out "semantic_$dir/p$i.c"
    echo "----------------------------------------"
done