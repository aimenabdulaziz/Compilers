# Makefile for the miniC compiler
#
# This Makefile compiles the miniC compiler, which includes lexical analysis,
# syntax analysis, Abstract Syntax Tree (AST) generation, and semantic analysis.
# It also provides targets for testing and running the compiler with Valgrind.
#
# Variables:
#   - SRC: Source file prefix for the miniC compiler
#   - TEST_PROG: Name of the test script
#   - CXX: C++ compiler
#   - CXXFLAGS: Compiler flags
#
# Targets:
#   - all: Builds the miniC compiler executable
#   - test: Runs the test script with the miniC compiler
#   - valgrind: Runs the miniC compiler with Valgrind to check for memory leaks
#   - clean: Removes build artifacts
#
# Usage:
#   - make: Build the miniC compiler executable
#   - make test: Run the test script
#   - make valgrind: Run the compiler with Valgrind
#   - make clean: Clean the build artifacts
#
# Author: Aimen Abdulaziz
# Date: Spring 2023

# Variables
SRC = miniC
TEST_PROG = testing.sh
CXX=g++
CXXFLAGS=-std=c++20 -Wextra -Wpedantic

# Targets
all: $(SRC).out

$(SRC).out: $(SRC).l $(SRC).y 
	yacc -d -v $(SRC).y
	lex $(SRC).l
	$(CXX) $(CXXFLAGS) -g -o miniC_main.out lex.yy.c y.tab.c ast.c semantic_analysis.c miniC_main.c

test: $(TEST_PROG) $(SRC).out
	chmod a+x $(TEST_PROG)
	bash -v ./$(TEST_PROG)

valgrind: $(SRC).out
	valgrind --leak-check=full --show-leak-kinds=all ./$(SRC).out lex_yacc_tests/p1.c

clean:
	rm -f *~ *.o *.out lex.yy.c y.tab.c y.tab.h y.output

.PHONY: all test valgrind clean 
