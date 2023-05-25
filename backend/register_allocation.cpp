#include "register_allocation.h"

static void
computeLiveness(LLVMBasicBlockRef basicBlock,
                LiveUsageMap &liveUsageMap,
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

        // Check if the instruction does NOT generate result (noResultOpCode is defined in register_allocation.h)
        LLVMOpcode instrOpcode = LLVMGetInstructionOpcode(instruction);
        if (noResultOpCode.find(instrOpcode) == noResultOpCode.end())
        {
            // Add the instruction to the live range of the instructions
            liveUsageMap[instruction].push_back(instructionList.size());
        }

        // Update the live usage of any of the operands in the instruction
        for (auto i = 0; i < LLVMGetNumOperands(instruction); i++)
        {
            LLVMValueRef operand = LLVMGetOperand(instruction, i);
            if (liveUsageMap.find(operand) != liveUsageMap.end())
            {
                liveUsageMap[operand].push_back(instructionList.size());
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
removeAllocatedRegister(int instructionIdx,
                        int operandStartIdx,
                        LLVMValueRef instruction,
                        LiveUsageMap &liveUsageMap,
                        InstIndex &instructionIndex,
                        AllocatedReg &bbAllocatedRegisterMap,
                        RegisterSet &availableRegisters)
{
    // Get the number of operands of the instruction
    int numOperands = LLVMGetNumOperands(instruction);

    // Iterate over each operand of the instruction
    for (int i = operandStartIdx; i < numOperands; i++)
    {
        // Get the i-th operand of the instruction
        LLVMValueRef operand = LLVMGetOperand(instruction, i);

        // Skip constants and operands that are not in the liveUsageMap
        if (LLVMIsAConstant(operand) || liveUsageMap.find(operand) == liveUsageMap.end())
        {
            continue;
        }

        // Check if the live range of the operand ends after the current instruction
        if (liveUsageMap[operand].back() > instructionIdx)
        {
            // If the live range of the operand does not end, continue to the next operand
            continue;
        }

        // Check if the operand has a physical register assigned to it
        auto it = bbAllocatedRegisterMap.find(operand);
        if (it == bbAllocatedRegisterMap.end())
        {
            // If the operand does not have a physical register assigned to it, continue to the next operand
            continue;
        }

        // Get the physical register assigned to the operand
        Register registerName = it->second;

        // Add the physical register to the set of available registers
        availableRegisters.insert(registerName);
    }
}

static LLVMValueRef
selectSpillInstr(LiveUsageMap &liveUsageMap,
                 AllocatedReg &bbAllocatedRegisterMap,
                 LLVMValueRef currInstr)
{
    LLVMValueRef spill_var = nullptr;
    int min_frequency = INT_MAX;
    for (auto it = bbAllocatedRegisterMap.begin(); it != bbAllocatedRegisterMap.end(); ++it)
    {
        LLVMValueRef instr = it->first;
        Register registerName = it->second;
        if (registerName != SPILL && liveUsageMap[instr].size() < min_frequency)
        {
            spill_var = instr;
            min_frequency = liveUsageMap[instr].size();
        }
    }
    return spill_var;
}

static void
printLiveUsageMap(LiveUsageMap &liveUsageMap)
{
    // Print the live range of each instruction
    for (auto usages : liveUsageMap)
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

std::string
getRegisterName(Register reg)
{
    switch (reg)
    {
    case EBX:
        return "EBX";
    case ECX:
        return "ECX";
    case EDX:
        return "EDX";
    case SPILL:
        return "SPILL";
    default:
        return "Unknown register";
    }
}

static void
mergeBBWGlobalMap(AllocatedReg &bbAllocatedRegisterMap,
                  AllocatedReg &allocatedRegisterMap)
{
    for (auto &entry : bbAllocatedRegisterMap)
    {
        LLVMValueRef instr = entry.first;
        Register reg = entry.second;

#ifdef DEBUG
        char *str = LLVMPrintValueToString(instr);
        cout << "Assigned register " << getRegisterName(reg) << " to" << str << endl;
        LLVMDisposeMessage(str);
#endif

        allocatedRegisterMap[instr] = reg;
    }
    cout << endl;
}

static void
allocateRegisterForBasicBlock(LLVMBasicBlockRef &basicBlock,
                              InstIndex &instructionList,
                              LiveUsageMap &liveUsageMap,
                              AllocatedReg &allocatedRegisterMap)
{
    // Create a map of instruction to register for the current basic block
    AllocatedReg bbAllocatedRegisterMap;
    RegisterSet availableRegisters = {EBX, ECX, EDX};

    for (int i = 0; i < instructionList.size(); i++)
    {
        LLVMValueRef currInstr = instructionList[i];
        LLVMOpcode instrOpcode = LLVMGetInstructionOpcode(currInstr);

        // Check if the instruction does NOT generates a result (no LHS)
        if (noResultOpCode.find(instrOpcode) != noResultOpCode.end())
        {
            // Check and remove allocated registers for each operand whose live range ends in the current instruction
            removeAllocatedRegister(i, 0, currInstr, liveUsageMap, instructionList, bbAllocatedRegisterMap, availableRegisters);
            continue;
        }

        // Check if the instruction is an arithmetic instruction
        if (arithmeticOpcode.find(instrOpcode) != arithmeticOpcode.end())
        {
            // Get the first operand of the instruction
            LLVMValueRef firstOperand = LLVMGetOperand(currInstr, 0);

            // Check if the first operand has a physical register assigned to it in reg_map
            // And the liveness range of the first operand ends at the instruction
            if (bbAllocatedRegisterMap.find(firstOperand) != bbAllocatedRegisterMap.end() && liveUsageMap[firstOperand].back() == i)
            {
                // Get the physical register assigned to the first operand
                Register reg = bbAllocatedRegisterMap[firstOperand];

                // Add the reg to the curr instr
                bbAllocatedRegisterMap[currInstr] = reg;
            }
            // If so, we try to remove the allocated register for the first operand
            removeAllocatedRegister(i, 1, currInstr, liveUsageMap, instructionList, bbAllocatedRegisterMap, availableRegisters);
            continue;
        }

        // Allocate a register to the instruction if there is an available register
        if (!availableRegisters.empty())
        {
            Register registerName = *availableRegisters.begin();
            // cout << "Allocating " << registerName << " to ";
            availableRegisters.erase(registerName);
            bbAllocatedRegisterMap[currInstr] = registerName;
            removeAllocatedRegister(i, 0, currInstr, liveUsageMap, instructionList, bbAllocatedRegisterMap, availableRegisters);
            continue;
        }

        // If there is no available register, find the instr to spill
        LLVMValueRef spillInstr = selectSpillInstr(liveUsageMap, bbAllocatedRegisterMap, currInstr);

        // Spill the curr instr if the spill instr has more uses than the curr instr
        if (liveUsageMap[spillInstr].size() > liveUsageMap[currInstr].size())
        {
            // Spill curr instr
            bbAllocatedRegisterMap[currInstr] = SPILL;
            continue;
        }
        else
        {
            // Get the register allocated to spillInstr
            Register spilledRegisterName = bbAllocatedRegisterMap[spillInstr];

            // Allocate the register to curr instr and update the register for spillInstr to "SPILL"
            bbAllocatedRegisterMap[currInstr] = spilledRegisterName;
            bbAllocatedRegisterMap[spillInstr] = SPILL;
        }

        removeAllocatedRegister(i, 0, currInstr, liveUsageMap, instructionList, bbAllocatedRegisterMap, availableRegisters);
    }

    // Merge the basic block allocated register map into the global allocated register map
    mergeBBWGlobalMap(bbAllocatedRegisterMap, allocatedRegisterMap);
}

void allocateRegisterForFunction(LLVMValueRef function)
{
    // Create a map to store the register allocated to each instruction
    AllocatedReg allocatedRegisterMap;

    // Get the first basic block
    LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
    while (basicBlock)
    {
        // Create an instruction index for the basic block
        InstIndex instructionList;
        LiveUsageMap liveUsageMap;
        computeLiveness(basicBlock, liveUsageMap, instructionList);

#ifdef DEBUG
        printInstructionIndexVector(instructionList);
        printLiveUsageMap(liveUsageMap);
#endif

        // Allocate register on a basic block level
        allocateRegisterForBasicBlock(basicBlock, instructionList, liveUsageMap, allocatedRegisterMap);

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