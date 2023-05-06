/*
 * optimizer.h - Header file for the optimizer module
 *
 * This module provides functionality to optimize LLVM IR code by applying
 * various optimization techniques, such as constant propagation, constant folding,
 * common subexpression elimination, and dead code elimination. It provides
 * functions to optimize a single function or the entire program (LLVM module).
 *
 * Functions:
 *  - optimizeFunction: Optimizes a single LLVM function
 *  - optimizeProgram: Optimizes the entire program (LLVM module)
 *
 * Author: Aimen Abdulaziz
 * Date: Spring 2023
 */

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <llvm-c/Core.h>

/**
 * @brief Optimizes a single LLVM function using various optimization techniques.
 *
 * This function performs multiple optimizations on the given LLVM function, including
 * constant propagation, constant folding, common subexpression elimination, and
 * dead code elimination. Optimizations are applied iteratively until no more changes
 * are made to the function.
 *
 * @param function The LLVM function to be optimized.
 */
void optimizeFunction(LLVMValueRef function);

/**
 * @brief Optimizes the entire program (LLVM module) by optimizing each function within.
 *
 * This function iterates through all functions in the provided LLVM module
 * and optimizes each of them by calling the optimizeFunction function.
 *
 * @param module The LLVM module representing the program to be optimized.
 */
void optimizeProgram(LLVMModuleRef module);

#endif // OPTIMIZER_H
