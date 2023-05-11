# CS57 - Compilers
## Aimen Abdulaziz (F0055HS), Spring 2023

## Building and Running the Program

1. Clone the repository and navigate to the root directory.
2. Make sure the `build.sh` script is executable. If not, run the following command to make it executable:
```bash
chmod a+x build.sh
```
3. Run the `build.sh` script with the input filename (without the `.c` extension) and an optional `DEBUG` flag to enable debug outputs. For example:
```bash
./build.sh miniC DEBUG
```

where `miniC` is the miniC filename. It is important that you invoke the script without the `.c` extension.

This will run `make clean` and `make` with or without the `DEBUG=1` flag for both the frontend and optimization projects, depending on whether you provide the `DEBUG` flag.

The script will then:
1. Build the frontend executable with or without debug flag
2. Parse the MiniC using Lex and Yacc
3. Perform semantic analysis on the MiniC file
4. Exit if the analysis fails
5. Generate LLVM Intermediate Representation (IR) code (using ir_generator.cpp and clang)
6. Build the optimization executable with or without debug flag
7. Optimize the generated IR code

## Outputs
- The LLVM IR generator will create a IR Code of the input file and save the output in a file named `basename_manual.ll`, where `basename` is derived from the input file, in the same directory as the input file, containing the LLVM IR code generated for the given miniC program.
- The optimizer module will process the input LLVM IR code, apply various optimizations, and generate an optimized output file in the same directory as the input file. The output file will have the same name as the input file, but with an `_optimized` postfix.

MiniC

- Can't have more than one expression in one line.
- MiniC always have two external declarations 
- [will update later]

