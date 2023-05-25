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

// Type definitions for the register allocation algorithm
typedef std::unordered_map<LLVMValueRef, std::vector<int>> LiveUsageMap;
typedef std::vector<LLVMValueRef> InstIndex;
typedef std::unordered_map<LLVMValueRef, int> RegMap;

// Enumeration of available registers
enum Register
{
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

// Set of opcodes that do not have a result value (LHS)
std::unordered_set<LLVMOpcode> noResultOpCode = {LLVMStore, LLVMBr, LLVMCall, LLVMRet};

// Set of arithmetic opcodes
std::unordered_set<LLVMOpcode> arithmeticOpcode = {LLVMAdd, LLVMSub, LLVMMul};

// Function declarations
void allocateRegisterForModule(LLVMModuleRef module);
void allocateRegisterForFunction(LLVMValueRef function);

#endif // REGISTER_ALLOCATION_H