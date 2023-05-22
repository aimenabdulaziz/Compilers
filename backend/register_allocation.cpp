#include "register_allocation.h"

static void populateInstructionIndex(LLVMBasicBlockRef basicBlock, vector<LLVMValueRef> &instructionIndex)
{
    LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock);

    while (instruction != NULL)
    {
        // Skip alloca instructions
        if (LLVMIsAAllocaInst(instruction))
        {
            instruction = LLVMGetNextInstruction(instruction);
            continue;
        }

        // Add the instruction to the index
        instructionIndex.push_back(instruction);

        // Get the next instruction
        instruction = LLVMGetNextInstruction(instruction);
    }
}

static void computeLiveness(LLVMBasicBlockRef basicBlock,
                            unordered_map<LLVMValueRef, vector<int>> &liveRange,
                            vector<LLVMValueRef> &instructionIndex)
{
    for (auto i = 0; i < instructionIndex.size(); i++)
    {
        LLVMValueRef instruction = instructionIndex[i];

        // Get the number of operands of the instruction
        int numOperands = LLVMGetNumOperands(instruction);

        // Iterate over the operands and add the instruction index to the live range of each operand
        for (auto j = 0; j < numOperands; j++)
        {
            // Get the operand
            LLVMValueRef operand = LLVMGetOperand(instruction, j);

            if (LLVMIsAConstant(operand))
            {
                // Skip constants
                continue;
            }

            // Add the instruction index to the live range of the operand
            liveRange[operand].push_back(i);
        }
    }
}

void allocateRegisterForFunction(LLVMValueRef function)
{
    LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
    while (basicBlock != NULL)
    {
        // Initialize the set of available registers
        unordered_set<string> availableRegisters = {"ebx", "ecx", "edx"};

        // Create an instruction index for the basic block
        vector<LLVMValueRef> instructionIndex;
        populateInstructionIndex(basicBlock, instructionIndex);

#ifdef DEBUG
        // print the instruction index vector
        for (auto i = 0; i < instructionIndex.size(); i++)
        {
            // Print the LLVMValueRef to a string
            char *instruction = LLVMPrintValueToString(instructionIndex[i]);
            cout << i << ": " << instruction << endl;
            LLVMDisposeMessage(instruction);
        }
        cout << endl;
#endif

        // Compute the liveness of each instruction and store it liveRange
        unordered_map<LLVMValueRef, vector<int>> liveRange;
        computeLiveness(basicBlock, liveRange, instructionIndex);

#ifdef DEBUG
        // Print the live range of each instruction
        for (auto usages : liveRange)
        {
            // print the vector of all the usages
            cout << "Live range of ";
            char *instruction = LLVMPrintValueToString(usages.first);
            cout << instruction << ": ";
            LLVMDisposeMessage(instruction);
            for (auto usage : usages.second)
            {
                cout << usage << " ";
            }
            cout << endl;
        }
#endif
        // Get the next basic block
        basicBlock = LLVMGetNextBasicBlock(basicBlock);
    }
}

void allocateRegisterForModule(LLVMModuleRef module)
{
    LLVMValueRef function = LLVMGetFirstFunction(module);
    while (function != NULL)
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
    if (module == NULL)
    {
        cout << "Error: Invalid LLVM IR file" << endl;
        return 2;
    }

    // Perform register allocation
    allocateRegisterForModule(module);

    return 0;
}