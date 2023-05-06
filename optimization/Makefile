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

# Uncomment the following line to enable debugging
# DEBUG = 1

ifeq ($(DEBUG), 1)
    CXXFLAGS = -g -DDEBUG
else
    CXXFLAGS = -g
endif

$(OPTIMIZER): $(OPTIMIZER).cpp
	clang++ $(CXXFLAGS) `llvm-config-15 --cxxflags` -I /usr/include/llvm-c-15/ -x c++ -c $(OPTIMIZER).cpp
	clang++ $(CXXFLAGS) `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/ $(OPTIMIZER).o -o $@

clean: 
	rm -rf $(OPTIMIZER)
	rm -rf *.o *.out
	
.PHONY: clean