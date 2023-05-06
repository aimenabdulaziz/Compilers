## MiniC Optimizer

## Usage
To use the Makefile for the optimizer module, follow the steps below:
1. Run `make` to build the optimizer executable. This will generate an executable named `optimizer`.
2. To enable debugging, you can uncomment the `DEBUG = 1` line in the Makefile. Alternatively, You can also pass it as an argument when invoking make, like this:
```bash
make DEBUG=1
``` 
3. Run the optimizer executable with the input LLVM IR file. For example: 
```bash 
./optimizer input.ll
```
The optimizer module will process the input LLVM IR code, apply various optimizations, and generate an optimized output file in the same directory as the input file. The output file will have the same name as the input file, but with an `_optimized` postfix.
4. To clean up the build artifacts, run `make clean`. This will remove the optimizer executable and any object files created during the build process.