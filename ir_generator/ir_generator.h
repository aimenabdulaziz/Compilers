/*
 * ir_builder.h - LLVM Intermediate Representation (IR) Builder Header
 *
 * This header file provides the interface for generating LLVM IR code from an Abstract Syntax Tree (AST) node.
 * 
 * Usage: Include "ir_generator.h" in your project and use the provided functions to generate LLVM IR from a miniC AST.
 *
 * Output: The program generates an output file named 'basename_manual.ll', where 'basename' is derived from the input file,
 *         in the same directory as the input file, containing the LLVM IR code generated for the given miniC program.
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
