# Lexical Analyzer and Parser

This directory contains the Lex and Yacc files for building a compiler for the MiniC language, a simplified subset of the C programming language. 

## Usage

1. Clone this repository or download the source files.
2. Navigate to the directory containing the source files.
3. Run `make` to build the MiniC lexical analyzer and parser.
4. Use the following command to compile a MiniC source file: `./miniC_main.out <input_file>`. Replace `<input_file>` with the path to your MiniC source file.
5. The compiled output will be printed on the terminal.

## Semantic Analysis

Semantic Analysis is performed by traversing the AST and checking for undeclared variables. The semantic analysis program print error messages with the undeclared variable name if the test fails. If there is no error message, it means the semantic analysis has successfully passed. The error message looks as follows: `Error: undeclared variable '<var>'` where `<var>` is the variable name. Please not that the same error message will be repeated multiple times if the variable is used more than once.


## Testing

To test the MiniC lexical analyzer and parser with a sample MiniC program, run `make test`. This will run the series of testing miniC programs in the `tests` directory. I strongly recommend redirecting the output to another file.
```bash
make test &> testing.out
```

## Clean Up

To clean up the generated files, run `make clean`. This will remove any compiled output, temporary files, and logs.