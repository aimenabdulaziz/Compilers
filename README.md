# CS57 - Compilers
## Aimen Abdulaziz (F0055HS), Spring 2023

## Building and Running the Program

1. Clone the repository and navigate to the root directory.
2. Make sure the build.sh script is executable. If not, run the following command to make it executable:
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
- Run the semantic analysis on the input file.
- Generate LLVM IR code for the input file.
- Optimize the LLVM IR code using the optimizer.

MiniC

- Can't have more than one expression in one line.
- MiniC always have two external declarations 
- [will update later]

