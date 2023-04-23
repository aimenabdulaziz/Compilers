# MiniC Compiler Makefile
# Author: Aimen Abdulaziz
# Date: Spring 2023
#
# This Makefile compiles and links the Lex and Yacc files to generate an 
# executable. It also provides targets to run tests and clean the build artifacts.

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
