# MiniC Compiler Makefile
# Author: Aimen Abdulaziz
# Date: April 9, 2023
#
# This Makefile compiles and links the Lex and Yacc files to generate an 
# executable. It also provides targets to run tests and clean the build artifacts.

# Variables
SRC = miniC
TEST_PROG = testing_mini.c

# Targets
all: $(SRC).out

$(SRC).out: $(SRC).l $(SRC).y
	yacc -d -v $(SRC).y
	lex $(SRC).l
	gcc -o $(SRC).out lex.yy.c y.tab.c

test: $(SRC).out
	./$(SRC).out $(TEST_PROG)

clean:
	rm -f *~ *.o lex.yy.c y.tab.c y.tab.h $(SRC).out y.output

.PHONY: all test clean
