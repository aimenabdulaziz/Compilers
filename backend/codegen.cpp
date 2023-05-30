/**
 * @file codegen.cpp
 * @brief A program that generates x86 assembly code from LLVM IR.
 *
 * This program takes an LLVM IR file as input, generates x86 assembly code from the IR, and writes the assembly code to a file.
 * The program uses the LLVM C API to parse the IR file, generate assembly code, and perform register allocation. The generated
 * assembly code is compatible with the GNU assembler and can be assembled and linked into an executable file.
 *
 * The program consists of several functions that are responsible for different parts of the code generation process. The main
 * function reads the input file, creates an LLVM module from the IR, and calls the `generateAssemblyCode` function to generate
 * assembly code for the module. The `generateAssemblyCode` function iterates through all functions in the module, allocates
 * registers for each function, and calls the `generateAssemblyForFunction` function to generate assembly code for the function.
 * The `generateAssemblyForFunction` function generates assembly code for the basic blocks in the function, and calls the
 * `generateAssemblyForInstructions` function to generate assembly code for the instructions in each basic block.
 *
 * The program is designed to be modular and extensible, and can be easily modified to support additional LLVM IR features or
 * target architectures. The program is written in C++ and uses the standard library and the LLVM C API.
 *
 * Usage:
 *   ./codegen <input_file>
 *   <input_file>  - The LLVM IR file to generate assembly code from.
 *
 * Output: The program writes the generated assembly code to a file with the same name as the input file but with a .s extension.
 *          If the input file is named `input.ll`, the program writes the assembly code to a file named `input.s`.
 *
 * Assumption:
 *  - The input LLVM IR file contains at most one function.
 *  - The function in the input LLVM IR file has at most one parameter.
 * These assumptions are consistent with the MiniC programming language.
 *
 * @author Aimen Abdulaziz
 * @date Spring 2023
 */

#include "codegen.h"

/**
 * @brief Create a label for a basic block.
 * @param basicBlock Reference to LLVM basic block.
 * @param bbLabelMap Map of basic block labels.
 */
static void
createBBLabel(LLVMBasicBlockRef &basicBlock, BasicBlockLabelMap &bbLabelMap)
{
    // Create label for basic block and store in bbLabelMap
    bbLabelMap[LLVMBasicBlockAsValue(basicBlock)] = ".L" + std::to_string(bbLabelMap.size());
}

/**
 * @brief Open an output file with a new extension.
 * @param filename Input file name.
 * @return Opened output file stream.
 */
std::ofstream
openOutputFile(const char *filename)
{
    // Save the output to a file with the same name as the input file but with a .s extension
    std::string outName;
    changeFileExtension(filename, outName, ".s");
    std::ofstream outputFile(outName);

    // Check if output file was opened successfully
    if (!outputFile.is_open())
    {
        std::cerr << "Could not open " << outName << " for writing.\n";
        return std::ofstream(); // Return an invalid ofstream object
    }

    return outputFile;
}

/**
 * @brief Print top-level assembly directives to output stream.
 * @param out Output stream.
 * @param filename Source filename.
 */
static void
printTopLevelDirective(std::ostream &out, const std::string &filename)
{
    // Output the .file directive along with the name of the source file
    out << "\t.file "
        << "\"" << filename << "\"" << std::endl;
    out << "\t.text" << std::endl; // Output the .text directive
}

/**
 * @brief Print function-specific assembly directives to output stream.
 *
 * This function prints the assembly directives that are specific to a function to the output stream. The directives include
 * specifying that the function is global, specifying the type of the function, setting up the stack frame, and allocating
 * space for the reserved registers and local variables. If the EBX register is used in the function, its value is saved.
 *
 * @param context The code generation context.
 * @param out The output stream to print the directives to.
 * @param functionName The name of the function.
 */
static void
printFunctionDirectives(CodeGenContext &context, std::ostream &out, const std::string &functionName)
{
    out << "\t.globl " << functionName << std::endl;                 // Specify that the function is global
    out << "\t.type " << functionName << ", @function" << std::endl; // Specify the type of the function

    // Specify the beginning of the function
    out << functionName << ":" << std::endl;
    out << ".LFB" << std::to_string(context.funCounter) << ":" << std::endl;

    // Setup the stack frame
    out << "\tpushl %ebp\n";

    // Set the new base pointer
    out << "\tmovl %esp, %ebp\n";

    // If the EBX register is used in the function, save its value
    if (context.usedEBX)
    {
        out << "\tpushl %ebx\n";
    }

    // Allocate space for the reserved registers and local variables
    out << "\tsubl $" << context.localMem << ", %esp\n";
}

/**
 * @brief Print assembly instructions to end a function.
 * @param out Output stream.
 */
static void
printFunctionEnd(std::ostream &out)
{
    out << "\tleave\n"; // Restore the stack frame
    out << "\tret\n";   // Return from the function
}

/**
 * @brief Check if instruction is an Alloca instruction.
 * @param instruction LLVM instruction.
 * @return True if instruction is an Alloca instruction, false otherwise.
 */
static bool
isAlloca(LLVMValueRef instruction)
{
    return LLVMGetInstructionOpcode(instruction) == LLVMAlloca;
}

/**
 * @brief Check if an instruction has been spilled to memory.
 *
 * This function checks if an LLVM instruction has been spilled to memory by looking up the instruction in the `allocatedRegMap`
 * map and checking if its value is `SPILL`. If the instruction has been spilled, the function returns `true`. Otherwise, it
 * returns `false`.
 *
 * @param instruction The LLVM instruction to check.
 * @param allocatedRegMap A map of LLVM instructions to their allocated registers.
 * @return `true` if the instruction has been spilled to memory, `false` otherwise.
 */
static bool
isSpilledInstruction(LLVMValueRef instruction, AllocatedReg &allocatedRegMap)
{
    if (allocatedRegMap.find(instruction) != allocatedRegMap.end())
    {
        if (allocatedRegMap[instruction] == SPILL)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Throw an error with a message and print the unhandled LLVM instruction.
 * @param ptr LLVM value.
 * @param message Error message.
 */
static void
throwError(LLVMValueRef ptr, std::string message)
{
    char *instr = LLVMPrintValueToString(ptr);
    std::cout << "Unhandled " << message << ". Value: " << instr << std::endl;
    LLVMDisposeMessage(instr);
}

/**
 * @brief Check if an LLVM instruction stores an argument.
 *
 * This function checks if an LLVM instruction stores an argument by iterating through all uses of the instruction and checking
 * if the stored value is an argument. If the stored value is an argument, the function returns `true`. Otherwise, it returns
 * `false`.
 *
 * @param instruction The LLVM instruction to check.
 * @return `true` if the instruction stores an argument, `false` otherwise.
 */
static bool
isParameter(LLVMValueRef instruction)
{
    // Iterate through all uses of the instruction
    for (LLVMUseRef use = LLVMGetFirstUse(instruction); use != NULL; use = LLVMGetNextUse(use))
    {
        LLVMValueRef user = LLVMGetUser(use);

        // Check if the user is a store instruction
        if (LLVMIsAStoreInst(user))
        {
            LLVMValueRef storedValue = LLVMGetOperand(user, 0);

            // Check if the stored value is an argument
            if (LLVMIsAArgument(storedValue))
            {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Populate the offset map for a basic block.
 *
 * This function populates the offset map for a basic block by iterating through all instructions in the basic block and adding
 * alloca instructions and spilled instructions to the offset map. If an instruction stores an argument, its offset is set to 8
 * bytes (for the return address and first parameter). Otherwise, the local memory offset is incremented by 4 bytes for each
 * alloca instruction, and the instruction is added to the offset map with a negative offset.
 *
 * @param basicBlock The basic block to populate the offset map for.
 * @param allocatedRegMap A map of LLVM instructions to their allocated registers.
 * @param usedEBX A flag indicating whether the EBX register is used in the basic block.
 * @param offsetMap The offset map to populate.
 * @param localMem The current local memory offset.
 */
static void
populateOffsetMap(LLVMBasicBlockRef basicBlock, AllocatedReg &allocatedRegMap,
                  bool &usedEBX, OffsetMap &offsetMap, int &localMem)
{
    // Get the first instruction in the basic block
    LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock);

    // The offset step is 4 bytes
    static const int offsetStep = 4;

    while (instruction)
    {
        // Add the instruction to the offset map if it is an alloca instruction or a spilled instruction
        if (isAlloca(instruction) || isSpilledInstruction(instruction, allocatedRegMap))
        {
            if (isParameter(instruction))
            {
                // 8 bytes for the return address and first parameter
                offsetMap[instruction] = offsetStep * 2;
            }
            else
            {
                // Increment the local memory offset
                localMem += offsetStep;

                // Add the alloca instruction to the offset map with a negative offset
                offsetMap[instruction] = -localMem;
            }
        }

        // Get the next instruction
        instruction = LLVMGetNextInstruction(instruction);
    }
}

/**
 * @brief Print the offset map to standard output.
 *
 * This function prints the offset map to standard output for debugging purposes. Each entry in the offset map is printed as a
 * string representation of the instruction followed by its offset.
 *
 * @param offsetMap The offset map to print.
 */
static void
printOffsetMap(OffsetMap &offsetMap)
{
    for (auto &entry : offsetMap)
    {
        char *instr = LLVMPrintValueToString(entry.first);
        std::cout << instr << ": " << entry.second << std::endl;
        LLVMDisposeMessage(instr);
    }
}

/**
 * @brief Check if an LLVM value is stored in a register.
 *
 * This function checks if an LLVM value is stored in a register by looking up the value in the `allocatedRegMap` map and checking
 * if its value is not `SPILL`. If the value is stored in a register, the function returns `true`. Otherwise, it returns `false`.
 *
 * @param context The code generation context.
 * @param value The LLVM value to check.
 * @return `true` if the value is stored in a register, `false` otherwise.
 */
static bool
variableIsInRegister(CodeGenContext &context, LLVMValueRef value)
{
    return (context.allocatedRegMap.count(value) > 0 && context.allocatedRegMap[value] != SPILL);
}

/**
 * @brief Check if an LLVM value is stored in memory.
 *
 * This function checks if an LLVM value is stored in memory by looking up the value in the `offsetMap` map. If the value is in
 * the `offsetMap` map, the function returns `true`. Otherwise, it returns `false`.
 *
 * @param context The code generation context.
 * @param value The LLVM value to check.
 * @return `true` if the value is stored in memory, `false` otherwise.
 */
static bool
variableIsInMemory(CodeGenContext &context, LLVMValueRef value)
{
    return (context.offsetMap.count(value) > 0);
}

/**
 * @brief Print an LLVM value to a stream.
 *
 * This function prints an LLVM value to a stream by converting the value to a string representation using the `LLVMPrintValueToString`
 * function and writing the string to the stream.
 *
 * @param value The LLVM value to print.
 * @param out The output stream to write to.
 */
static void
printLLVMValueRef(LLVMValueRef value, std::ostream &out)
{
    char *instr = LLVMPrintValueToString(value);
    out << instr << "\n";
    LLVMDisposeMessage(instr);
}

/**
 * @brief Get the assembly opcode for an LLVM integer predicate.
 *
 * This function returns the assembly opcode for an LLVM integer predicate. The function takes an `LLVMIntPredicate` value as input
 * and returns a string representation of the corresponding assembly opcode. If the predicate is not supported, the function prints
 * an error message and returns `nullptr`.
 *
 * @param predicate The LLVM integer predicate to get the assembly opcode for.
 * @return The assembly opcode as a string, or `nullptr` if the predicate is not supported.
 */
static std::string
getAssemblyOpcodeForPredicate(LLVMIntPredicate predicate)
{
    switch (predicate)
    {
    case LLVMIntEQ:
        return "je";
    case LLVMIntNE:
        return "jne";
    case LLVMIntSGT:
        return "jg";
    case LLVMIntSGE:
        return "jge";
    case LLVMIntSLT:
        return "jl";
    case LLVMIntSLE:
        return "jle";
    default:
    {
        cout << "Unsupported comparison predicate\n";
        return nullptr;
    }
    }
}

/**
 * @brief Get the assembly opcode for an LLVM instruction.
 *
 * This function returns the assembly opcode for an LLVM instruction. The function takes an `LLVMValueRef` value as input and returns
 * a string representation of the corresponding assembly opcode. If the instruction is not supported, the function prints an error
 * message and returns `nullptr`.
 *
 * @param instruction The LLVM instruction to get the assembly opcode for.
 * @return The assembly opcode as a string, or `nullptr` if the instruction is not supported.
 */
static std::string
getAssemblyOpcodeForInstruction(LLVMValueRef instruction)
{
    LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);
    switch (opcode)
    {
    case LLVMAdd:
        return "addl";
    case LLVMSub:
        return "subl";
    case LLVMMul:
        return "imull";
    case LLVMICmp:
        return "cmpl";
    default:
    {
        cout << "Unsupported opcode\n";
        return nullptr;
    }
    }
}

/**
 * @brief Handle the LLVMRet opcode.
 *
 * This function handles the LLVMRet opcode by generating assembly code to move the return value into the `%eax` register. If the
 * return value is a constant integer, the function generates code to move the integer value into `%eax`. If the return value is
 * stored in memory, the function generates code to move the value from the memory location into `%eax`. If the return value is
 * stored in a register, the function generates code to move the value from the register into `%eax`.
 *
 * @param instruction The LLVM instruction to handle.
 * @param context The code generation context.
 */
static void
handleLLVMRet(LLVMValueRef instruction, CodeGenContext &context)
{
    static std::ostream &out = context.outputFile;
    LLVMValueRef returnValue = LLVMGetOperand(instruction, 0);

    if (LLVMIsAConstantInt(returnValue))
    {
        int value = LLVMConstIntGetSExtValue(returnValue);
        out << "\tmovl $" << value << ", %eax\n";
    }
    else if (variableIsInMemory(context, returnValue))
    {
        int offset = context.offsetMap[returnValue];
        out << "\tmovl " << offset << "(%ebp), %eax\n";
    }
    else if ((variableIsInRegister(context, returnValue)))
    {
        Register reg = context.allocatedRegMap[returnValue];
        out << "\tmovl %" << getRegisterName(reg) << ", %eax\n";
    }
#ifdef DEBUG
    else
    {
        throwError(returnValue, "return");
    }
#endif
}

/**
 * @brief Handle the LLVMLoad opcode.
 *
 * This function handles the LLVMLoad opcode by generating assembly code to move the value from the memory location into a register.
 * If the value is stored in a register, the function generates code to move the value from the register into the destination
 * register.
 *
 * @param instruction The LLVM instruction to handle.
 * @param context The code generation context.
 */
static void
handleLLVMLoad(LLVMValueRef instruction, CodeGenContext &context)
{
    static std::ostream &out = context.outputFile;

    if (variableIsInRegister(context, instruction))
    {
        LLVMValueRef loadValue = LLVMGetOperand(instruction, 0);
        int offset = context.offsetMap[loadValue];
        Register reg = context.allocatedRegMap[instruction];
        out << "\tmovl " << offset << "(%ebp), %" << getRegisterName(reg) << "\n";
    }
    else if (variableIsInMemory(context, instruction))
    {
        LLVMValueRef loadValue = LLVMGetOperand(instruction, 0);
        int offset1 = context.offsetMap[loadValue];
        int offset2 = context.offsetMap[instruction];
        out << "\tmovl " << offset1 << "(%ebp), %eax\n";
        out << "\tmovl %eax, " << offset2 << "(%ebp)\n";
    }
#ifdef DEBUG
    else
    {
        throwError(instruction, "load");
    }
#endif
}

/**
 * @brief Handle the LLVMStore opcode.
 *
 * This function handles the LLVMStore opcode by generating assembly code to move the value to the memory location. If the value
 * is a constant integer, the function generates code to move the integer value to the memory location. If the value is stored
 * in a register, the function generates code to move the value from the register to the memory location. If the value is stored
 * in memory, the function generates code to move the value from the source memory location to the destination memory location.
 *
 * @param instruction The LLVM instruction to handle.
 * @param context The code generation context.
 */
static void
handleLLVMStore(LLVMValueRef instruction, CodeGenContext &context)
{
    static std::ostream &out = context.outputFile;
    LLVMValueRef storedValue = LLVMGetOperand(instruction, 0);
    LLVMValueRef storeLocation = LLVMGetOperand(instruction, 1);

    if (isParameter(storedValue))
    {
        // Do nothing
    }
    else if (LLVMIsAConstantInt(storedValue))
    {
        int offset = context.offsetMap[storeLocation];
        int value = LLVMConstIntGetSExtValue(storedValue);
        out << "\tmovl $" << value << ", " << offset << "(%ebp)\n";
    }
    else if (variableIsInRegister(context, storedValue))
    {
        Register reg = context.allocatedRegMap[storedValue];
        int offset = context.offsetMap[storeLocation];
        out << "\tmovl %" << getRegisterName(reg) << ", " << offset << "(%ebp)\n";
    }
    else
    {
        int offset1 = context.offsetMap[storedValue];
        int offset2 = context.offsetMap[storeLocation];
        out << "\tmovl " << offset1 << "(%ebp), %eax\n";
        out << "\tmovl %eax, " << offset2 << "(%ebp)\n";
    }
}

/**
 * @brief Handle an LLVM call instruction.
 *
 * This function handles an LLVM call instruction by pushing the registers onto the stack, getting the called function and the
 * number of parameters, and pushing the parameter onto the stack if it is a constant, in a register, or in memory. The function
 * then emits the function invoked by the call instruction and pops the registers off the stack. If the function returns an
 * integer, the function moves the result to a register or memory location.
 *
 * @param instruction The LLVM call instruction to handle.
 * @param context The code generation context.
 */
static void
handleLLVMCall(LLVMValueRef instruction, CodeGenContext &context)
{
    static std::ostream &out = context.outputFile;

    // Push the registers onto the stack
    out << "\tpushl %ebx\n";
    out << "\tpushl %ecx\n";
    out << "\tpushl %edx\n";

    // Get the called function
    LLVMValueRef func = LLVMGetCalledValue(instruction);

    // Get the number of parameters
    int numParams = LLVMCountParams(func);

    if (numParams > 0)
    {
        // MiniC always has one parameter
        LLVMValueRef param = LLVMGetOperand(instruction, 0);

        // Check if param is a constant
        if (LLVMIsAConstant(param))
        {
            int value = LLVMConstIntGetSExtValue(param);
            out << "pushl $" << value << endl;
        }
        else if (variableIsInRegister(context, param))
        {
            Register reg = context.allocatedRegMap[param];
            out << "\tpushl %" << getRegisterName(reg) << "\n";
        }
        else if (variableIsInMemory(context, param))
        {
            int offset = context.offsetMap[param];
            out << "\tpushl " << offset << "(%ebp)\n";
        }
#ifdef DEBUG
        else
        {
            throwError(instruction, "call");
        }
#endif
    }

    // Emit the function invoked by the call instruction
    const char *funcName = LLVMGetValueName(func);
    out << "\tcall " << funcName << "@PLT\n";

    if (numParams > 0)
    {
        // Undo the offset of pushing the parameter of func
        out << "\taddl $" << 4 << ", %esp\n";
    }

    // Pop the registers off the stack
    out << "\tpopl %edx\n";
    out << "\tpopl %ecx\n";
    out << "\tpopl %ebx\n";

    LLVMTypeRef returnType = LLVMTypeOf(instruction);

    if (LLVMGetTypeKind(returnType) == LLVMIntegerTypeKind)
    {
        // The function returns an integer (read() in MiniC)
        if (variableIsInRegister(context, instruction))
        {
            Register reg = context.allocatedRegMap[instruction];
            out << "\tmovl %eax, %" << getRegisterName(reg) << "\n";
        }
        else if (variableIsInMemory(context, instruction))
        {
            int offset = context.offsetMap[instruction];
            out << "\tmovl %eax, " << offset << "(%ebp)\n";
        }
#ifdef DEBUG
        else
        {
            throwError(instruction, "call. The variable is not in register or memory");
        }
#endif
    }
}

/**
 * @brief Handle an LLVM branch instruction.
 *
 * This function handles an LLVM branch instruction by checking if the instruction is conditional or unconditional. If the
 * instruction is conditional, the function gets the comparison predicate, the true and false labels, and emits the corresponding
 * assembly opcode. If the instruction is unconditional, the function gets the branch label and emits a jump instruction to the
 * label.
 *
 * @param instruction The LLVM branch instruction to handle.
 * @param context The code generation context.
 */
static void
handleLLVMBr(LLVMValueRef instruction, CodeGenContext &context)
{
    static std::ostream &out = context.outputFile;
    if (LLVMIsConditional(instruction))
    {
        LLVMValueRef condition = LLVMGetOperand(instruction, 0);
        std::string falseLabel = context.bbLabelMap[LLVMGetOperand(instruction, 1)];
        std::string trueLabel = context.bbLabelMap[LLVMGetOperand(instruction, 2)];

        // Get the comparison predicate
        LLVMIntPredicate predicate = LLVMGetICmpPredicate(condition);

        // Get the corresponding assembly opcode
        std::string jmpInstruction = getAssemblyOpcodeForPredicate(predicate);

        out << "\t" << jmpInstruction << " " << trueLabel << "\n";
        out << "\tjmp " << falseLabel << "\n";
    }
    else
    {
        // Get branch label
        std::string label = context.bbLabelMap[LLVMGetOperand(instruction, 0)];
        out << "\tjmp " << label << "\n";
    }
}

/**
 * @brief Handle binary and comparison instructions.
 *
 * This function handles binary and comparison instructions by getting the first and second operands, checking if they are
 * constants, in registers, or in memory, and emitting the corresponding assembly opcode. If the first operand is in memory, the
 * function moves the result to a register or memory location.
 *
 * @param instruction The LLVM instruction to handle.
 * @param context The code generation context.
 */
static void
handleBinaryAndComparisonInstructions(LLVMValueRef instruction, CodeGenContext &context)
{
    // Code to handle the LLVMAdd, LLVMSub, LLVMMul, and LLVMICmp opcodes
    static std::ostream &out = context.outputFile;
    Register operationReg;

    if (variableIsInRegister(context, instruction))
    {
        operationReg = context.allocatedRegMap[instruction];
    }
    else
    {
        operationReg = EAX;
    }

    // If the first operand is a constant, move it to operationReg
    LLVMValueRef operand1 = LLVMGetOperand(instruction, 0);
    if (LLVMIsAConstant(operand1))
    {
        int value = LLVMConstIntGetSExtValue(operand1);
        out << "\tmovl $" << value << ", %" << getRegisterName(operationReg) << "\n";
    }
    else if (variableIsInRegister(context, operand1))
    {
        Register reg = context.allocatedRegMap[operand1];

        if (getRegisterName(reg) != getRegisterName(operationReg))
        {
            out << "\tmovl %" << getRegisterName(reg) << ", %" << getRegisterName(operationReg) << "\n";
        }
    }
    else if (variableIsInMemory(context, operand1))
    {
        int offset = context.offsetMap[operand1];
        out << "\tmovl " << offset << "(%ebp), %" << getRegisterName(operationReg) << "\n";
    }

    LLVMValueRef operand2 = LLVMGetOperand(instruction, 1);
    if (LLVMIsAConstant(operand2))
    {
        int value = LLVMConstIntGetSExtValue(operand2);
        out << "\t" << getAssemblyOpcodeForInstruction(instruction) << " $" << value << ", %" << getRegisterName(operationReg) << "\n";
    }
    else if (variableIsInRegister(context, operand2))
    {
        Register reg = context.allocatedRegMap[operand2];
        out << "\t" << getAssemblyOpcodeForInstruction(instruction) << " %" << getRegisterName(reg) << ", %" << getRegisterName(operationReg) << "\n";
    }
    else if (variableIsInMemory(context, operand2))
    {
        int offset = context.offsetMap[operand2];
        out << "\t" << getAssemblyOpcodeForInstruction(instruction) << " " << offset << "(%ebp), %" << getRegisterName(operationReg) << "\n";
    }
    // else if (variableIsInMemory(context, operand2))
    // {
    //     int offset = context.offsetMap[operand2];
    //     out << "\tmovl " << offset << "(%ebp), %ebx\n"; // Load the spilled value into a different register, let's say EBX for this example
    //     out << "\t" << getAssemblyOpcodeForInstruction(instruction) << " %ebx, %" << getRegisterName(operationReg) << "\n";
    // }

    // If the instruction ptr is in memory, move the result to the memory location
    if (variableIsInMemory(context, instruction))
    {
        int offset = context.offsetMap[instruction];
        out << "\tmovl %" << getRegisterName(operationReg) << ", " << offset << "(%ebp)\n";
    }
}

/**
 * @brief Generates assembly code for the instructions in a given basic block.
 *
 * This function generates assembly code for the instructions in a given basic block. It first emits the basic block label to the
 * output file, and then iterates through all instructions in the basic block. For each instruction, the function determines its
 * opcode and calls the appropriate handler function to generate the corresponding assembly code.
 *
 * @param basicBlock The basic block to generate assembly code for.
 * @param context The code generation context to use.
 */
static void
generateAssemblyForInstructions(LLVMBasicBlockRef basicBlock, CodeGenContext &context)
{
    // Emit basic block label
    static std::ostream &out = context.outputFile;
    std::string label = context.bbLabelMap[LLVMBasicBlockAsValue(basicBlock)];
    if (label != ".L0")
    {
        out << label << ":\n";
    }

    // Iterate through all instructions in the basic block
    LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock);
    while (instruction)
    {
        LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);
        switch (opcode)
        {
        case LLVMRet:
        {
            handleLLVMRet(instruction, context);
            break;
        }
        case LLVMLoad:
        {
            handleLLVMLoad(instruction, context);
            break;
        }
        case LLVMStore:
        {
            handleLLVMStore(instruction, context);
            break;
        }
        case LLVMCall:
        {
            handleLLVMCall(instruction, context);
            break;
        }
        case LLVMBr:
        {
            handleLLVMBr(instruction, context);
            break;
        }
        case LLVMAlloca:
        {
            // Do nothing
            break;
        }
        case LLVMAdd:
        case LLVMSub:
        case LLVMMul:
        case LLVMICmp:
        {
            handleBinaryAndComparisonInstructions(instruction, context);
            break;
        }
        default:
        {
            throwError(instruction, "default");
            break;
        }
        }

        instruction = LLVMGetNextInstruction(instruction);
    }
}

/**
 * @brief Generates assembly code for the basic blocks in a given code generation context.
 *
 * This function generates assembly code for the basic blocks in a given code generation context. It first emits the function
 * directives to the output file, and then iterates through all basic blocks in the function. For each basic block, the function
 * calls the `generateAssemblyForInstructions` function to generate assembly code for the instructions in the basic block. After
 * generating assembly code for all basic blocks, the function emits the function end directive to the output file.
 *
 * @param context The code generation context to generate assembly code for.
 */
static void
generateAssemblyForBasicBlocks(CodeGenContext &context)
{
    LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(context.function);

    if (basicBlock)
    {
        printFunctionDirectives(context, context.outputFile, LLVMGetValueName(context.function));
    }

    // Iterate through the basic blocks in the function
    while (basicBlock)
    {
        // Iterate over the instructions and generate assembly
        generateAssemblyForInstructions(basicBlock, context);

        // Get the next basic block
        basicBlock = LLVMGetNextBasicBlock(basicBlock);
    }

    printFunctionEnd(context.outputFile);
}

/**
 * @brief Generates assembly code for a given LLVM function.
 *
 * This function generates assembly code for a given LLVM function. It first initializes data structures to store the basic block
 * labels and the offsets of the local variables, and then iterates through all basic blocks in the function. For each basic block,
 * the function calls the `createBBLabel` and `populateOffsetMap` functions to generate the basic block label and calculate the
 * offset of the local variables. After initializing the code generation context, the function calls the `generateAssemblyForBasicBlocks`
 * function to generate assembly code for the basic blocks in the function.
 *
 * @param function The LLVM function to generate assembly code for.
 * @param allocatedRegMap A map of allocated registers for the function.
 * @param outputFile The output file to write the assembly code to.
 * @param usedEBX A flag indicating whether the EBX register is used in the function.
 * @param funCounter The index of the function in the module.
 */
static void
generateAssemblyForFunction(LLVMValueRef function, AllocatedReg &allocatedRegMap, std::ofstream &outputFile, bool &usedEBX, const int &funCounter)
{
    LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);

    if (!basicBlock)
    {
        return;
    }

    // Create data structures to store the basic block labels and the offsets of the local variables
    static BasicBlockLabelMap bbLabelMap;
    static OffsetMap offsetMap;

    // Keep track of the number of basic blocks in the function
    int bbCounter = 0;

    // Keep track of the offset of the local variables
    int localMem = 0;

    if (usedEBX)
    {
        localMem += 4; // 4 bytes for the EBX register
    }

    while (basicBlock)
    {
        createBBLabel(basicBlock, bbLabelMap);

        populateOffsetMap(basicBlock, allocatedRegMap, usedEBX, offsetMap, localMem);

        // Get the next basic block and increment the basic block label counter
        basicBlock = LLVMGetNextBasicBlock(basicBlock);
        bbCounter++;
    }

#ifdef DEBUG
    printOffsetMap(offsetMap);
    cout << "Local memory: " << localMem << endl;
#endif

    CodeGenContext context(function, bbLabelMap, allocatedRegMap, offsetMap, outputFile, usedEBX, funCounter, localMem);
    generateAssemblyForBasicBlocks(context);
}

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
void generateAssemblyCode(LLVMModuleRef module, const char *filename)
{
    LLVMValueRef function = LLVMGetFirstFunction(module);

    // Save the output to a file with the same name as the input file but with a .s extension
    std::ofstream outputFile = openOutputFile(filename);
    printTopLevelDirective(outputFile, filename);

    int funCounter = 0;
    while (function)
    {
        // Allocate registers for the function
        bool usedEBX = false;
        AllocatedReg allocatedRegMap = allocateRegisterForFunction(function, usedEBX);

        // Generate assembly code for the function
        generateAssemblyForFunction(function, allocatedRegMap, outputFile, usedEBX, funCounter);

        // Get the next function and increment the function counter
        function = LLVMGetNextFunction(function);
        funCounter++;
    }
}

/**
 * @brief The entry point of the program.
 *
 * This function is the entry point of the program. It checks the number of command-line arguments and creates an LLVM module from
 * the specified IR file. If the module is valid, it calls the `generateAssemblyCode` function to generate assembly code for the
 * module. If the module is invalid, the function prints an error message and returns a non-zero exit code.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 * @return An integer representing the exit code of the program.
 */
int main(int argc, char **argv)
{
    // Check the number of arguments
    if (argc != 2)
    {
        cout << "Usage: " << argv[0] << " <filename.ll>" << endl;
        return 1;
    }

    // Create LLVM module from IR file
    LLVMModuleRef module = createLLVMModel(argv[1]);

    // Check if module is valid
    if (!module)
    {
        cout << "Error: Invalid LLVM IR file" << endl;
        return 2;
    }

    // Perform register allocation
    generateAssemblyCode(module, argv[1]);

    return 0;
}