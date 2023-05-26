#include "codegen.h"

void generateAssemblyCode(LLVMModuleRef module)
{
    LLVMValueRef function = LLVMGetFirstFunction(module);
    while (function)
    {
        // Allocate registers for the function
        AllocatedReg allocatedRegMap = allocateRegisterForFunction(function);

        // Get the next function
        function = LLVMGetNextFunction(function);
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
    generateAssemblyCode(module);

    return 0;
}