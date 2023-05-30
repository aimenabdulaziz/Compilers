# CS57 - Compilers
## Aimen Abdulaziz (F0055HS), Spring 2023

## Building and Running the Program

1. Clone the repository and navigate to the root directory.
2. Make sure the `build.sh` script is executable. If not, run the following command to make it executable:
```bash
chmod a+x build.sh
```
1. Run the `build.sh` script with the input filename and an optional `DEBUG` flag to enable debug outputs. For example:
```bash
./build.sh test.c
```
or 

```bash
./build.sh test.c DEBUG
```

where `test.c` is the MiniC filename. 

This will run `make clean` and `make` with or without the `DEBUG=1` flag for the frontend, optimization, and backend module, depending on whether you provide the `DEBUG` flag.

The script will then:
1. Checks the number of command-line arguments, exits with an error if incorrect
2. Builds the frontend, optimization, and backend executables, with optional debug flag
3. Parses the MiniC file using Lex and Yacc
4. Performs semantic analysis on the MiniC file and exits if the analysis fails
5. Generates LLVM Intermediate Representation (IR) code from the parsed MiniC file
6. Optimizes the generated IR code
7. Generates x86 assembly code from the optimized IR
8. Cleans up by removing the executables

## Outputs
- The LLVM IR generator will create a IR Code of the input file and save the output in a file named `basename_manual.ll`, where `basename` is derived from the input file, in the same directory as the input file, containing the LLVM IR code generated for the given miniC program.
- The optimizer module will process the input LLVM IR code, apply various optimizations, and generate the optimized LLVM IR code is save to a file named `basename_opt.ll` in the same directory as the input file.
- The backend module writes the generated assembly code to a file with the same name as the input file but with a `.s` extension. If the input file is named `input.ll`, the program writes the assembly code to a file named `input.s`. The `build.sh` script will take care of passing the optimize LLVM IR code to the backend executable.


## MiniC Features and Restrictions
### General

- The MiniC language only supports compiling a single file program, which consists of a single function definition.
- The function can be named anything other than `main`, `print`, and `read`.
There are two external function declarations, `print` and `read`, that are used for printing and reading values. No explicit calls to `printf` or `scanf` are allowed.
- Comments are not allowed in MiniC. Comments will result in parsing error.

### Functions

- Functions can only have a maximum of one parameter.
- Functions can only return an integer.
- The only external functions that can be called are `print` and `read`. No system calls are included. It is expected that both of these external functions are imported each MiniC file even if they might not be used.

### Variables

- All variables (local variables and parameters) are integers.
- Local variables are declared at the beginning of a block.
- Each variable should be declared separately, in a new line.
- Variables are initialized after all the variables have been declared. MiniC doesn't support variable declaration and initialization in the same line.

### Operations

- Arithmetic operations are limited to addition (+), subtraction (-), multiplication (*), and division (/). However, assembly code generation doesn't support division.
- Each arithmetic operation can only have two operands.
- Comparison operators allowed in `if` and `while` conditions are: greater than (>), less than (<), equals (==), greater than or equals to (>=), less than or equals to (<=). Logical operators (&&, ||, !) are not used. Additionally, comparison between two expressions is not allowed -- comparison should only be between terms.


## Repository Organization

The MiniC compiler consists of three major components: `frontend`, `optimization`, and `backend`. The `common` directory contains common modules shared across files. This repository is organized as such:

```bash
├── backend
│   ├── codegen.cpp
│   ├── codegen.h
│   ├── Makefile
│   ├── README.md
│   ├── register_allocation.cpp
│   ├── register_allocation.h
│   └── testing.sh
├── build.sh
├── common
│   ├── common.a
│   ├── file_utils.cpp
│   ├── file_utils.h
│   └── Makefile
├── frontend
│   ├── ast.cpp
│   ├── ast.h
│   ├── ast_test.c
│   ├── frontend
│   ├── frontend.cpp
│   ├── frontend.l
│   ├── frontend.y
│   ├── IMPLEMENTATION.md
│   ├── Makefile
│   ├── README.md
│   ├── semantic_analysis.cpp
│   ├── semantic_analysis.h
│   └── testing.sh
├── ir_generator
│   ├── ir_generator.cpp
│   ├── ir_generator.h
│   └── README.md
├── LICENSE
├── optimization
│   ├── Makefile
│   ├── optimizer.cpp
│   ├── optimizer.h
│   ├── README.md
│   └── testing.sh
├── README.md
├── testing.sh              # automated test for the whole compiler
└── tests                   # each directory below has its own tests
    ├── backend             
    ├── ir_generator
    ├── lex_yacc
    ├── optimization
    └── semantic_analysis
```