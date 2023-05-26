SRC = codegen
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

$(SRC): $(SRC).cpp $(LLIBS) register_allocation.o
	clang++ $(CXXFLAGS) `llvm-config-15 --cxxflags ` -x c++ -c $<
	clang++ $(CXXFLAGS) `llvm-config-15 --cxxflags --ldflags --libs core` $^ -o $@

test: $(TEST_PROG) $(OPTIMIZER).out
	chmod a+x $(TEST_PROG)
	bash -v ./$(TEST_PROG)

clean: 
	rm -rf $(SRC)
	rm -rf *.o *.out
	
.PHONY: test clean