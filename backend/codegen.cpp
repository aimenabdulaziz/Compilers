// Assumption: MiniC has at most one parameter

#include "codegen.h"

static void createBBLabel(LLVMBasicBlockRef &basicBlock, BasicBlockLabelMap &bbLabelMap)
{
    bbLabelMap[LLVMBasicBlockAsValue(basicBlock)] = ".L" + std::to_string(bbLabelMap.size());
}

std::ofstream openOutputFile(const char *filename)
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

static void printTopLevelDirective(std::ostream &out, const std::string &filename)
{
    out << "\t.file "                             // Output the .file directive
        << "\"" << filename << "\"" << std::endl; // Specify the name of the source file
    out << "\t.text" << std::endl;
}

static void printFuncDirectives(std::ostream &out, const std::string functionName, bool &usedEBX, const int &funCounter, const int &localMem)
{
    out << "\t.globl " << functionName << std::endl;                 // Specify that the function is global
    out << "\t.type " << functionName << ", @function" << std::endl; // Specify the type of the function

    // Specify the beginning of the function
    out << functionName << ":" << std::endl;
    out << ".LFB" << std::to_string(funCounter) << ":" << std::endl;

    // Setup the stack frame
    out << "\tpushl %ebp\n";

    // Set the new base pointer
    out << "\tmovl %esp, %ebp\n";

    // If the EBX register is used in the function, save its value
    if (usedEBX)
    {
        out << "\tpushl %ebx\n";
    }

    // Allocate space for the reserved registers and local variables
    out << "\tsubl $" << localMem << ", %esp\n";
}

static void printFunctionEnd(std::ostream &out, std::string functionName, const int &funCounter)
{
    out << "\tleave\n"; // Restore the stack frame
    out << "\tret\n";   // Return from the function
}

static bool isAlloca(LLVMValueRef instruction)
{
    return LLVMGetInstructionOpcode(instruction) == LLVMAlloca;
}

static bool isSpilledInstruction(LLVMValueRef instruction, AllocatedReg &allocatedRegMap)
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

static void throwError(LLVMValueRef ptr, std::string message)
{
    char *instr = LLVMPrintValueToString(ptr);
    std::cout << "Unhandled " << message << ". Value: " << instr << std::endl;
    LLVMDisposeMessage(instr);
}
static bool isParameter(LLVMValueRef instruction)
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

static void populateOffsetMap(LLVMBasicBlockRef basicBlock, AllocatedReg &allocatedRegMap,
                              bool &usedEBX, OffsetMap &offsetMap, int &localMem)
{
    // Get the first instruction in the basic block
    LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock);

    // The offset step is 4 bytes
    static const int offsetStep = 4;

    while (instruction)
    {
        // Add the instruction to the offset map if it is an alloca instruction
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

                // Add the alloca instruction to the offset map
                offsetMap[instruction] = -localMem;
            }
        }

        // Get the next instruction
        instruction = LLVMGetNextInstruction(instruction);
    }
}

static void printOffsetMap(OffsetMap &offsetMap)
{
    for (auto &entry : offsetMap)
    {
        char *instr = LLVMPrintValueToString(entry.first);
        std::cout << instr << ": " << entry.second << std::endl;
        LLVMDisposeMessage(instr);
    }
}

static bool variableIsInRegister(CodeGenContext &context, LLVMValueRef value)
{
    return (context.allocatedRegMap.count(value) > 0 && context.allocatedRegMap[value] != SPILL);
}

static bool variableIsInMemory(CodeGenContext &context, LLVMValueRef value)
{
    return (context.offsetMap.count(value) > 0);
}

static void printLLVMValueRef(LLVMValueRef value, std::ostream &out)
{
    char *instr = LLVMPrintValueToString(value);
    out << instr << "\n";
    LLVMDisposeMessage(instr);
}

std::string getAssemblyOpcodeForPredicate(LLVMIntPredicate predicate)
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
        cout << "Unsupported comparison predicate\n";
        return nullptr;
    }
}
static void
generateAssemblyForInstructions(LLVMBasicBlockRef basicBlock, CodeGenContext &context)
{
    std::ostream &out = context.outputFile;
    LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock);
    while (instruction)
    {
        LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);
        switch (opcode)
        {
        case LLVMRet:
        {
            // Code to handle the LLVMRet opcode
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
            break;
        }
        case LLVMLoad:
        {
            // Code to handle the LLVMLoad opcode
            if (variableIsInRegister(context, instruction))
            {
                LLVMValueRef loadValue = LLVMGetOperand(instruction, 0);

                // Load instruction has the same offset as its operand
                int offset = context.offsetMap[loadValue];

                Register reg = context.allocatedRegMap[instruction];

                out << "\tmovl " << offset << "(%ebp), %" << getRegisterName(reg) << "\n";
            }
#ifdef DEBUG
            else
            {
                throwError(instruction, "load");
            }
#endif
            break;
        }
        case LLVMStore:
        {
            // Code to handle the LLVMStore opcode
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
            else if (variableIsInMemory(context, storedValue))
            {
                int offset1 = context.offsetMap[storedValue];
                int offset2 = context.offsetMap[storeLocation];
                out << "\tmovl " << offset1 << "(%ebp), %eax\n";
                out << "\tmovl %eax, " << offset2 << "(%ebp)\n";
            }
#ifdef DEBUG
            else
            {
                throwError(storedValue, "store");
            }
#endif
            break;
        }
        case LLVMCall:
        {
            // Code to handle the LLVMCall opcode
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

                printf("param: %s\n", LLVMPrintValueToString(param));
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
                out << "\taddl $" << 4 * numParams << ", %esp\n";
            }

            // Pop the registers
            out << "\tpopl %edx\n";
            out << "\tpopl %ecx\n";
            out << "\tpopl %ebx\n";

            LLVMTypeRef returnType = LLVMTypeOf(instruction);

            if (LLVMGetTypeKind(returnType) == LLVMIntegerTypeKind)
            {
                // The function return an integer (read() in MiniC)
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
            break;
        }
        case LLVMBr:
        {
            // Code to handle the LLVMBr opcode
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
            break;
        }
        case LLVMAlloca:
        {
            // Code to handle the LLVMAlloca opcode
            break;
        }
        case LLVMAdd:
        case LLVMSub:
        case LLVMMul:
        case LLVMICmp:
        {
            // Code to handle the LLVMAdd, LLVMSub, LLVMMul, and LLVMICmp opcodes
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
static void
generateAssemblyForBasicBlocks(CodeGenContext &context)
{
    LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(context.function);

    if (basicBlock)
    {
        printFuncDirectives(context.outputFile, LLVMGetValueName(context.function), context.usedEBX, context.funCounter, context.localMem);
    }

    // Iterate through the basic blocks in the function
    while (basicBlock)
    {
        // Iterate over the instructions and generate assembly
        generateAssemblyForInstructions(basicBlock, context);

        // Get the next basic block
        basicBlock = LLVMGetNextBasicBlock(basicBlock);
    }
}
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