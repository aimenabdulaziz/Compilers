/*
 * MiniC Compiler - LLVM IR Builder Header
 *
 **************** ADD DOCUMENTATION HERE *******************
 * 
 * Author: Aimen Abdulaziz
 * Date: Spring 2023
 */

#ifndef LLVM_IR_GENERATOR_H
#define LLVM_IR_GENERATOR_H

#include "ast.h"
#include <llvm-c/Core.h>

// Generate LLVM IR from AST
LLVMModuleRef generateLLVMIR(astNode *node, char *filename);

#endif // LLVM_IR_GENERATOR_H
