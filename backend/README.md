# Register Allocation Module
This module provides an implementation of register allocation for LLVM IR code using the linear scan algorithm.

## Usage
To use the register allocation module, include the register_allocation.h header file in your C++ code:
```bash
#include "register_allocation.h"
```

The module provides two functions for allocating registers:
```bash
void allocateRegistersForFunction(LLVMValueRef function);
void allocateRegistersForModule(LLVMModuleRef module);
```

The `allocateRegistersForFunction` function allocates registers for a single function. It takes a LLVMValueRef argument that represents the function to allocate registers for.

The `allocateRegistersForModule` function allocates registers for all functions in a module. It takes a LLVMModuleRef argument that represents the module to allocate registers for.

## Data Structures

The register allocation module defines several data structures:

#### LiveUsageMap
The `LiveUsageMap` data structure is a map that stores the live usage information for each instruction in a function. It is used to calculate the live usage frequency of operands.

#### InstIndex
The `InstIndex` data structure is a map that stores the index of each instruction in the function. It is used to calculate the live usage frequency of operands.

#### RegMap
The `RegMap` data structure is a map that stores the physical register assigned to each virtual register in the function.

#### RegisterSet
The `RegisterSet` data structure is a set that stores the available physical registers.

#### AllocatedReg
The `AllocatedReg` data structure is a map that stores the physical register assigned to each instruction operand in a basic block.

## Algorithm
The register allocation algorithm used in this module is the linear scan algorithm. The algorithm performs the following steps:
1. Computes liveness information for each basic block in the function.
2. Allocates registers for each basic block using the linear scan algorithm.
3. Removes allocated registers for operands whose live range ends in the current instruction.
4. If no registers are available, selects an instruction to spill based on the live usage frequency of its operands.
5. Merges the basic block allocated register map into the global allocated register map.