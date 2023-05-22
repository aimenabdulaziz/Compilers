#include "register_allocation.h"


static void populateInstructionIndex(LLVMBasicBlockRef basicBlock, vector<LLVMValueRef> &instructionIndex) {
    LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock);

    while (instruction != NULL) {
        // Skip alloca instructions
        if (LLVMIsAAllocaInst(instruction)) {
            instruction = LLVMGetNextInstruction(instruction);
            continue;
        }

        // Add the instruction to the index
        instructionIndex.push_back(instruction);

        // Get the next instruction
        instruction = LLVMGetNextInstruction(instruction);

    }
}

void allocateRegisterForFunction(LLVMValueRef function) {
    LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
    while (basicBlock != NULL) {
        // Initialize the set of available registers
        unordered_set<string> availableRegisters = { "ebx", "ecx", "edx" };

        // Create an instruction index for the basic block
        vector<LLVMValueRef> instructionIndex;
        populateInstructionIndex(basicBlock, instructionIndex);

        #ifdef DEBUG
        // print the instruction index vector
        for (auto i = 0; i < instructionIndex.size(); i++) {
            // Print the LLVMValueRef to a string
            char* instruction = LLVMPrintValueToString(instructionIndex[i]);
            cout << i << ": " << instruction << endl;
            LLVMDisposeMessage(instruction);
        }
        #endif
        cout << endl;
        // Get the next basic block
        basicBlock = LLVMGetNextBasicBlock(basicBlock);
    }
}

void allocateRegisterForModule(LLVMModuleRef module) {
    LLVMValueRef function = LLVMGetFirstFunction(module);
    while (function != NULL) {
        // Allocate registers for the function
        allocateRegisterForFunction(function);

        // Get the next function
        function = LLVMGetNextFunction(function);
    }
}

// main function
int main(int argc, char** argv) {
    // Check the number of arguments
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <filename.ll>" << endl;
        return 1;
    }

    // Create LLVM module from IR file
    LLVMModuleRef module = createLLVMModel(argv[1]);

    // Check if module is valid
    if (module == NULL) {
        cout << "Error: Invalid LLVM IR file" << endl;
        return 2;
    }

    // Perform register allocation
    allocateRegisterForModule(module);

    return 0;
}