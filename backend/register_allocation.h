#include "file_utils.h"
#include <llvm-c/Core.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
using namespace std;

void allocateRegisterForModule(LLVMModuleRef module);
void allocateRegisterForFunction(LLVMValueRef function);