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
typedef unordered_map<LLVMValueRef, vector<int>> LiveRangeMap;
typedef std::vector<LLVMValueRef> InstIndex;
typedef std::unordered_map<LLVMValueRef, std::vector<int>> LiveRange;
typedef std::unordered_map<LLVMValueRef, int> RegMap;

// Enumeration of available registers
enum Register
{
    EBX,
    ECX,
    EDX,
    NUM_REGISTERS
};

// Set of available registers
typedef std::unordered_set<Register> RegisterSet;
RegisterSet availableRegisters = {EBX, ECX, EDX};

// Set of opcodes that do not have a result value (LHS)
std::unordered_set<LLVMOpcode> noResultOpCode = {LLVMStore, LLVMBr, LLVMCall, LLVMRet};

// Function declarations
void allocateRegisterForModule(LLVMModuleRef module);
void allocateRegisterForFunction(LLVMValueRef function);

#endif // REGISTER_ALLOCATION_H