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
IR_GENERATOR = llvm_ir_generator
CXX = g++
CXXFLAGS = -std=c++20 -Wextra -Wpedantic 
LLVM_CXXFLAGS = `llvm-config-15 --cxxflags`
LLVM_LDFLAGS = `llvm-config-15 --ldflags --libs core`
LLVM = /usr/include/llvm-c-15/

# Targets
.PHONY: all test valgrind clean ir_builder

all: $(SRC).out

$(IR_GENERATOR).o: $(IR_GENERATOR).cpp
	clang++ -g $(LLVM_CXXFLAGS) -I $(LLVM) -x c++ -c $< -o $@

$(SRC).out: $(SRC).l $(SRC).y $(IR_GENERATOR).o
	yacc -d -v $(SRC).y
	lex $(SRC).l
	$(CXX) $(CXXFLAGS) -g -o $(SRC).out lex.yy.c y.tab.c ast.c semantic_analysis.c miniC_main.c $(IR_GENERATOR).o $(LLVM_LDFLAGS) -I $(LLVM)


test: $(TEST_PROG) $(SRC).out
	chmod a+x $(TEST_PROG)
	bash -v ./$(TEST_PROG)

valgrind: $(SRC).out
	valgrind --leak-check=full --show-leak-kinds=all ./$(SRC).out lex_yacc_tests/p1.c

clean:
	rm -f *~ *.o *.out lex.yy.c y.tab.c y.tab.h y.output ir_builder.o
