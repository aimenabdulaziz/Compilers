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
