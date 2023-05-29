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
#   - make: Build the 'frontend' compiler executable
#   - make test: Run the test script
#   - make valgrind: Run the compiler with Valgrind
#   - make clean: Clean the build artifacts
#
# Author: Aimen Abdulaziz
# Date: Spring 2023

# Variables
SRC = frontend
TEST_PROG = testing.sh
IR_GENERATOR = ir_generator
IR_GENERATOR_DIR = ../ir_generator
C = ../common
LLIBS = $(C)/common.a
INCLUDES = -I $(LLVM) -I $(IR_GENERATOR_DIR) -I . -I $(C)
CXX = clang++
LLVM_LDFLAGS = `llvm-config-15 --ldflags --libs core`
LLVM = /usr/include/llvm-c-15/

ifeq ($(DEBUG), 1)
	CXXFLAGS = -g -DDEBUG -Wextra -Wpedantic -gdwarf-4 -Wno-deprecated $(INCLUDES) 
else
	CXXFLAGS = -g -Wextra -Wpedantic -gdwarf-4 -Wno-deprecated $(INCLUDES)
endif

# Targets
.PHONY: all test valgrind clean ir_builder

# Main target to build the output binary
all: $(SRC).out

# Rule for generating y.tab.h and y.tab.c files using yacc
y.tab.h y.tab.c: $(SRC).y
	yacc -d -v $(SRC).y

# Rule for generating lex.yy.c file using lex
lex.yy.c: $(SRC).l 
	lex $(SRC).l

# Rule for building the final output binary
$(SRC): lex.yy.c y.tab.c $(SRC).o $(IR_GENERATOR_DIR)/$(IR_GENERATOR).o ast.o semantic_analysis.o $(LLIBS)
	$(CXX) $(CXXFLAGS) $^ $(LLVM_LDFLAGS) -o $@

# Rule for building ast.o object file
ast.o: ast.cpp

# Rule for building semantic_analysis.o object file
semantic_analysis.o: semantic_analysis.cpp

# Rule for building frontend.o object file
$(SRC)o: $(SRC).cpp

# Rule for building the IR generator object file
$(IR_GENERATOR_DIR)/$(IR_GENERATOR).o: $(IR_GENERATOR_DIR)/$(IR_GENERATOR).cpp

# Target for running the test script
test: $(TEST_PROG) $(SRC).out
	chmod a+x $(TEST_PROG)
	bash -v ./$(TEST_PROG)

# Target for running the compiler with Valgrind
valgrind: $(SRC).out
	valgrind --leak-check=full --show-leak-kinds=all ./$(SRC).out ../tests/ir_generator/p2.c 

# Target for cleaning the build artifacts
clean:
	rm -f *~ *.o *.out* lex.yy.c y.tab.c y.tab.h $(IR_GENERATOR_DIR)/$(IR_GENERATOR).o
