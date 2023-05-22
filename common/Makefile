CC = clang++  # define the compiler to use
CFLAGS = -g -Wextra -Wpedantic -gdwarf-4 -Wno-deprecated -I /usr/include/llvm-c-15/ # define the flags to use for the compiler

# define the object file
OBJS = file_utils.o

# define the output library
LIB = common.a

all: $(LIB)

$(LIB): $(OBJS)
	ar rcs $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<  -o $@

clean:
	rm -f *.o
	rm -f $(LIB)
