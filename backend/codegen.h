#ifndef CODEGEN_H
#define CODEGEN_H

#include "register_allocation.h"
#include <fstream>

// Type definitions for data structures used in the code generation
typedef std::unordered_map<LLVMBasicBlockRef, std::string> BasicBlockLabelMap;
typedef std::unordered_map<LLVMValueRef, int> OffsetMap;

#endif // CODEGEN_H