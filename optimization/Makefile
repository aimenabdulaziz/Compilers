# Makefile for the optimizer module
#
# This Makefile compiles the optimizer module that provides functionality
# for optimizing LLVM IR code using various techniques. It handles the
# compilation of the C++ source code and linking with LLVM libraries.
#
# Targets:
#   - optimizer: Builds the optimizer executable
#   - clean: Removes the optimizer executable and other object files
#
# Configuration:
#   - OPTIMIZER: Name of the optimizer executable
#   - DEBUG: Enable debugging (uncomment the line to enable)
#   - CXXFLAGS: Compiler flags
#
# Usage:
#   - make: Build the optimizer executable
#   - make clean: Clean the build artifacts
#
# Author: Aimen Abdulaziz
# Date: Spring 2023

OPTIMIZER = optimizer
C = ../common
LLIBS = $(C)/common.a
LLVM = /usr/include/llvm-c-15/
INCLUDES = -I $(LLVM) -I $(C)
TEST_PROG = testing.sh


# Uncomment the following line to enable debugging
# DEBUG = 1

ifeq ($(DEBUG), 1)
    CXXFLAGS = -g -DDEBUG $(INCLUDES)
else
    CXXFLAGS = -g $(INCLUDES)
endif

$(OPTIMIZER): $(OPTIMIZER).cpp $(LLIBS)
	clang++ $(CXXFLAGS) `llvm-config-15 --cxxflags ` -x c++ -c $<
	clang++ $(CXXFLAGS) `llvm-config-15 --cxxflags --ldflags --libs core` $^ -o $@

test: $(TEST_PROG) $(OPTIMIZER).out
	chmod a+x $(TEST_PROG)
	bash -v ./$(TEST_PROG)

clean: 
	rm -rf $(OPTIMIZER)
	rm -rf *.o *.out
	
.PHONY: test clean