# MiniC Optimizer Makefile
# Author: Aimen Abdulaziz
# Date: Spring 2023

OPTIMIZER = optimizer
TEST = test

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

llvm_file: $(TEST).c
	clang -S -emit-llvm $(TEST).c -o $(TEST).ll

clean: 
	rm -rf $(OPTIMIZER)
	rm -rf *.o

.PHONY: llvm_file clean