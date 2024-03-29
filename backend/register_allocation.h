/**
 * @file register_allocation.h
 *
 * @brief Header file for register allocation algorithm for LLVM IR code using the linear scan algorithm.
 *
 * This file defines the data structures and function declarations for the register allocation algorithm.
 * The algorithm uses the linear scan algorithm to allocate registers for LLVM IR code.
 * The file includes type definitions for the LiveUsageMap, InstIndex, RegMap, RegisterSet, and AllocatedReg data structures.
 * It also defines the Register enumeration and sets of opcodes for instructions that do not have a result value and arithmetic opcodes.
 * The file provides function declarations for allocating registers for a single function and for all functions in a module.
 *
 * Usage: #include "register_allocation.h"
 *
 * @author Aimen Abdulaziz
 * @date Spring 2023
 */

#ifndef REGISTER_ALLOCATION_H
#define REGISTER_ALLOCATION_H

#include "file_utils.h"
#include <llvm-c/Core.h>
#include <iostream>
#include <string>
#include <vector>
#include <climits>
#include <unordered_set>
#include <unordered_map>

using namespace std;

// Type definitions for data structures used in the register allocation
typedef std::unordered_map<LLVMValueRef, std::vector<int>> LiveUsageMap;
typedef std::vector<LLVMValueRef> InstIndex;
typedef std::unordered_map<LLVMValueRef, int> RegMap;
typedef std::unordered_set<LLVMOpcode> OpcodeSet;

// Enumeration of available registers
enum Register
{
    EAX,
    EBX,
    ECX,
    EDX,
    NUM_REGISTERS,
    SPILL
};

// Set of available registers
typedef std::unordered_set<Register> RegisterSet;

// Map of allocated registers
typedef std::unordered_map<LLVMValueRef, Register> AllocatedReg;

/**
 * Returns the name of the given register as a string.
 *
 * @param reg The register to get the name of.
 * @return The name of the register as a string.
 */
std::string
getRegisterName(Register reg);

// Function declarations
/**
 * Allocates registers for the given LLVM function using the linear scan algorithm.
 * The function creates an AllocatedReg map to store the register allocated to each instruction.
 * It then iterates over each basic block in the function and allocates registers for each basic block.
 * The allocated registers are stored in the AllocatedReg map.
 *
 * @param function The LLVM function to allocate registers for.
 * @param usedEBX A flag to indicate if the EBX register is used in the function.
 * @return The AllocatedReg map that stores the register allocated to each instruction.
 */
AllocatedReg allocateRegisterForFunction(LLVMValueRef function, bool &usedEBX);

#endif // REGISTER_ALLOCATION_H