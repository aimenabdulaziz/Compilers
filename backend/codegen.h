/**
 * @file codegen.h
 * @brief Defines functions and data structures for generating assembly code from LLVM IR code.
 *
 * This file defines the `generateAssemblyCode` function and the `CodeGenContext` class for generating assembly code from LLVM IR code.
 *
 * The `generateAssemblyCode` function generates assembly code for a given LLVM module. It first initializes the output file and writes
 * the top-level directives to the output file. It then iterates through all functions in the module, allocating registers for each
 * function and calling the `generateAssemblyForFunction` function to generate assembly code for the function. After generating assembly
 * code for all functions, the function writes the top-level end directive to the output file and closes the output file.
 *
 * The `CodeGenContext` class contains the context for code generation. It contains the LLVM function, basic block label map, allocated
 * register map, offset map, output file stream, and other parameters needed for code generation. The `function` member variable is the
 * LLVM function to generate code for. The `bbLabelMap` member variable is a map that associates each basic block with a label. The
 * `allocatedRegMap` member variable is a map that associates each register with an LLVM value. The `offsetMap` member variable is a map
 * that associates each local variable with its offset in the stack frame. The `outputFile` member variable is a reference to the output
 * file stream for the generated code. The `usedEBX` member variable indicates whether the `EBX` register is used as a base pointer in
 * the stack frame. The `funCounter` member variable is a counter that is incremented for each function in the module. The `localMem`
 * member variable is the total size of the local variables in the stack frame.
 *
 * Type definitions for data structures used in the code generation are also provided. The `BasicBlockLabelMap` type is a map that
 * associates each basic block with a label. The `OffsetMap` type is a map that associates each LLVM value with its offset in the stack
 * frame.
 *
 * Usage : #include "codegen.h"
 *
 * @author Aimen Abdulaziz
 * @date Spring 2023
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "register_allocation.h"
#include <fstream>

// Type definitions for data structures used in the code generation
typedef std::unordered_map<LLVMValueRef, std::string>
    BasicBlockLabelMap;
typedef std::unordered_map<LLVMValueRef, int> OffsetMap; // ptr -> offset value

/**
 * @brief Generates assembly code for a given LLVM module.
 *
 * This function generates assembly code for a given LLVM module. It first initializes the output file and writes the top-level
 * directives to the output file. It then iterates through all functions in the module, allocating registers for each function and
 * calling the `generateAssemblyForFunction` function to generate assembly code for the function. After generating assembly code for
 * all functions, the function writes the top-level end directive to the output file and closes the output file.
 *
 * @param module The LLVM module to generate assembly code for.
 * @param filename The name of the output file to write the assembly code to.
 */
void generateAssemblyCode(LLVMModuleRef module, const char *filename);

/**
 * @brief A class that contains the context for code generation.
 *
 * This class contains the LLVM function, basic block label map, allocated register map,
 * offset map, output file stream, and other parameters needed for code generation.
 *
 * The `function` member variable is the LLVM function to generate code for.
 *
 * The `bbLabelMap` member variable is a map that associates each basic block with a label.
 *
 * The `allocatedRegMap` member variable is a map that associates each register with an LLVM value.
 *
 * The `offsetMap` member variable is a map that associates each local variable with its offset in the stack frame.
 *
 * The `outputFile` member variable is a reference to the output file stream for the generated code.
 *
 * The `usedEBX` member variable indicates whether the `EBX` register is used as a base pointer in the stack frame.
 *
 * The `funCounter` member variable is a counter that is incremented for each function in the module.
 *
 * The `localMem` member variable is the total size of the local variables in the stack frame.
 */
class CodeGenContext
{
public:
    CodeGenContext(LLVMValueRef function, BasicBlockLabelMap &bbLabelMap, AllocatedReg &allocatedRegMap, OffsetMap &offsetMap, std::ofstream &outputFile, bool usedEBX, int funCounter, int localMem)
        : function(function), bbLabelMap(bbLabelMap), allocatedRegMap(allocatedRegMap), offsetMap(offsetMap), outputFile(outputFile), usedEBX(usedEBX), funCounter(funCounter), localMem(localMem)
    {
    }

    LLVMValueRef function;
    BasicBlockLabelMap bbLabelMap;
    AllocatedReg allocatedRegMap;
    OffsetMap offsetMap;
    std::ofstream &outputFile;
    bool usedEBX;
    int funCounter;
    int localMem;
};

#endif // CODEGEN_H