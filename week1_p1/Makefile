# MiniC Compiler Makefile
# Author: Aimen Abdulaziz
# Date: April 9, 2023
#
# This Makefile compiles and links the Lex and Yacc files to generate an 
# executable. It also provides targets to run tests and clean the build artifacts.

# Variables
SRC = miniC
TEST_PROG = testing.sh

# Targets
all: $(SRC).out

$(SRC).out: $(SRC).l $(SRC).y
	yacc -d -v $(SRC).y
	lex $(SRC).l
	gcc -o $(SRC).out lex.yy.c y.tab.c

test: $(TEST_PROG) $(SRC).out
	chmod a+x $(TEST_PROG)
	bash -v ./$(TEST_PROG)

clean:
	rm -f *~ *.o *.out lex.yy.c y.tab.c y.tab.h y.output

.PHONY: all test clean
