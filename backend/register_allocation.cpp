#include "register_allocation.h"

static void
computeLiveness(LLVMBasicBlockRef basicBlock,
                LiveRange &liveUsages,
                InstIndex &instructionList)
{
    LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock);

    while (instruction)
    {
        // Skip alloca instructions
        if (LLVMIsAAllocaInst(instruction))
        {
            instruction = LLVMGetNextInstruction(instruction);
            continue;
        }

        // Check if the instruction generates a value
        std::unordered_set<LLVMOpcode> noValueOpCode = {LLVMStore, LLVMBr, LLVMCall, LLVMRet};
        LLVMOpcode instrOpcode = LLVMGetInstructionOpcode(instruction);
        if (noValueOpCode.find(instrOpcode) == noValueOpCode.end())
        {
            // Add the instruction to the live range of the instructions
            liveUsages[instruction].push_back(instructionList.size());
        }

        // Update the live usage of any of the operands in the instruction
        for (auto i = 0; i < LLVMGetNumOperands(instruction); i++)
        {
            LLVMValueRef operand = LLVMGetOperand(instruction, i);
            if (liveUsages.find(operand) != liveUsages.end())
            {
                liveUsages[operand].push_back(instructionList.size());
            }
        }

        // Add the instruction to the list
        instructionList.push_back(instruction);

        // Get the next instruction
        instruction = LLVMGetNextInstruction(instruction);
    }
}

static void
printInstructionIndexVector(InstIndex &instructionIndex)
{
    // print the instruction index vector
    for (auto i = 0; i < instructionIndex.size(); i++)
    {
        // Print the LLVMValueRef to a string
        char *instruction = LLVMPrintValueToString(instructionIndex[i]);
        cout << i << ": " << instruction << endl;
        LLVMDisposeMessage(instruction);
    }
    cout << endl;
}

static void
printLiveRange(LiveRange &liveRange)
{
    // Print the live range of each instruction
    for (auto usages : liveRange)
    {
        // print the vector of all the usages
        cout << "Live range of";
        char *instruction = LLVMPrintValueToString(usages.first);
        cout << instruction << ": ";
        LLVMDisposeMessage(instruction);
        for (auto usage : usages.second)
        {
            cout << usage << " ";
        }
        cout << endl;
    }
}

void allocateRegisterForFunction(LLVMValueRef function)
{
    // Create a map to store the register allocated to each instruction
    unordered_map<LLVMValueRef, string> allocatedRegisterMap;

    // Get the first basic block
    LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
    while (basicBlock)
    {
        // Create an instruction index for the basic block
        InstIndex instructionList;
        LiveRange liveUsages;
        computeLiveness(basicBlock, liveUsages, instructionList);

#ifdef DEBUG
        printInstructionIndexVector(instructionList);
        printLiveRange(liveUsages);
#endif

        // Get the next basic block
        basicBlock = LLVMGetNextBasicBlock(basicBlock);
    }
}

void allocateRegisterForModule(LLVMModuleRef module)
{
    LLVMValueRef function = LLVMGetFirstFunction(module);
    while (function)
    {
        // Allocate registers for the function
        allocateRegisterForFunction(function);

        // Get the next function
        function = LLVMGetNextFunction(function);
    }
}

// main function
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
    allocateRegisterForModule(module);

    return 0;
}