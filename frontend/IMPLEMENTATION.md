# MiniC Frontend Implementation Spec
## Aimen Abdulaziz, Spring 2023

## Data Structures 

For the Semantic Analysis, each symbol table is represented as a `std::set` of `std::string`. This choice ensures that the symbol table contains unique variable names and provides efficient lookup. The stack of symbol tables is represented as a `std::vector`.

## Semantic Analysis