#include "file_utils.h"
#include <llvm-c/Core.h>
#include <iostream>
#include <string>
#include <vector>
#include <climits>
#include <unordered_set>
#include <unordered_map>
using namespace std;

/* TODO: ADD A TYPE DEF FOR THE MAPS */
typedef unordered_map<LLVMValueRef, vector<int>> LiveRangeMap;
typedef std::vector<LLVMValueRef> InstIndex;
typedef std::unordered_map<LLVMValueRef, std::vector<int>> LiveRange;
typedef std::unordered_map<LLVMValueRef, int> RegMap;

enum Register
{
    EBX,
    ECX,
    EDX,
    NUM_REGISTERS
};

typedef std::unordered_set<Register> RegisterSet;

RegisterSet availableRegisters = {EBX, ECX, EDX};

void allocateRegisterForModule(LLVMModuleRef module);
void allocateRegisterForFunction(LLVMValueRef function);