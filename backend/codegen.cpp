#include "codegen.h"

static void createBBLabel(LLVMBasicBlockRef &basicBlock, BasicBlockLabelMap &bbLabelMap)
{
    bbLabelMap[basicBlock] = "bb" + std::to_string(bbLabelMap.size());
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
            // Increment the local memory offset
            localMem += offsetStep;

            // Add the alloca instruction to the offset map
            offsetMap[instruction] = localMem;
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

static void
generateAssemblyForInstructions(LLVMBasicBlockRef basicBlock)
{
    LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock);
    while (instruction)
    {
        LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);
        switch (opcode)
        {
        case LLVMRet:
        {
            // Code to handle the LLVMRet opcode
            break;
        }
        case LLVMLoad:
        {
            // Code to handle the LLVMLoad opcode
            break;
        }
        case LLVMStore:
        {
            // Code to handle the LLVMStore opcode
            break;
        }
        case LLVMCall:
        {
            // Code to handle the LLVMCall opcode
            break;
        }
        case LLVMBr:
        {
            // Code to handle the LLVMBr opcode
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
            char *instr = LLVMPrintValueToString(instruction);
            cout << "Unhandled instruction: " << instr << endl;
            LLVMDisposeMessage(instr);
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
        generateAssemblyForInstructions(basicBlock);

        // Get the next basic block
        basicBlock = LLVMGetNextBasicBlock(basicBlock);
    }
}
static void
generateAssemblyForFunction(LLVMValueRef function, AllocatedReg &allocatedRegMap, std::ofstream &outputFile, bool &usedEBX, const int &funCounter)
{
    // Create data structures to store the basic block labels and the offsets of the local variables
    BasicBlockLabelMap bbLabelMap;
    OffsetMap offsetMap;

    // Keep track of the number of basic blocks in the function
    int bbCounter = 0;
    LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);

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