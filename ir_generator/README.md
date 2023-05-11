# IR Generator for MiniC Compiler

The `ir_generator` is a module responsible for generating the Intermediate Representation (IR) code using LLVM for a MiniC compiler.

## Overview

The `ir_generator` takes an Abstract Syntax Tree (AST) as input and converts it into LLVM IR code. The LLVM IR code can then be further processed to create an executable, object file, or assembly code using other LLVM tools.

This module is designed to work alongside other components in the MiniC compiler, such as the `lexer`, `parser`, and `semantic analyzer`, to create a complete compilation process.

## Features
- Only support MiniC (a subset of the C programming language)
- Generates LLVM IR code for the given AST
- Saves the generated IR code to an output file named `<basename>_manual.ll`, where `<basename>` is derived from the input file, in the same directory as the input file.

## Usage
To use the `ir_generator`, you need to include the header file in your project:
```bash
#include "ir_generator.h"
```

After constructing the AST from the input source code and running semantic analysis, call the `generateIRAndSaveToFile` function with the root node of the AST and the name of the input file:
```bash
generateIRAndSaveToFile(root, "input_file_name");
```

The `generateIRAndSaveToFile` function will generate the LLVM IR code and save it to a file with the same name as the input file but with `_manual.ll` extension.