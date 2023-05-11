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

/**
 * Generates LLVM IR code from the given AST and saves it to a file with a '_manual.ll' extension.
 * 
 * @param node      The Abstract Syntax Tree (AST) node to generate LLVM IR code from.
 * @param filename  The input filename, used as the basis for the output file's name.
 * @return          Returns a pointer to the generated LLVM module, or nullptr if there was an error.
 */
LLVMModuleRef generateIRAndSaveToFile(astNode *node, const char *filename);

#endif // LLVM_IR_GENERATOR_H
