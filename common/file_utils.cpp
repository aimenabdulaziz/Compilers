/**
 * @file file_utils.cpp
 * 
 * @brief This file contains the definitions for utility functions used across the compiler. 
 * These include creating an LLVM model from a given filename and changing the file extension of a given filename.
 * These utilities are used in multiple stages of the compiler process.
 * 
 * The functions are defined in a separate compilation unit and can be linked into other components as a library.
 *
 * @author Aimen Abdulaziz
 * @date Spring 2023
 */

#include "file_utils.h"

/**
 * Create LLVM module from the given filename
 * @param filename Path to the LLVM IR file
 * @return LLVMModuleRef representing the module created from the given file, or NULL if error occurs
 */
LLVMModuleRef createLLVMModel(char *filename) {
    char *err = 0;

    LLVMMemoryBufferRef ll_f = 0;
    LLVMModuleRef m = 0;

    // Read the contents of the file into a memory buffer
    LLVMCreateMemoryBufferWithContentsOfFile(filename, &ll_f, &err);

    // If there was an error creating the memory buffer, print the error message and return NULL
    if (err != NULL) { 
        printf("Error creating memory buffer: %s\n", err);
        LLVMDisposeMessage(err);
        return NULL;
    }

    // Parse the LLVM IR in the memory buffer and create a new module
	LLVMParseIRInContext(LLVMGetGlobalContext(), ll_f, &m, &err);
    if (err != NULL) {
        printf("Error parsing LLVM IR: %s\n", err);
        LLVMDisposeMessage(err);
        return NULL;
    }
	
	// Return the LLVM module
    return m;
}

/**
 * @brief Changes the file extension of the given filename.
 *
 * This function takes the input filename and changes its extension to what is passed in
 * the `fileExtension` argument.
 * The modified filename is stored in the output parameter.
 *
 * @param[in] filename The input filename as a C-style string (const char*).
 * @param[in] fileExtension The new file extension as a std::string.
 * @param[out] output The modified filename with the new extension as a reference to a std::string.
 */
void changeFileExtension(const char *filename, string &output, string fileExtension) {
	// Convert the input C-style string filename to a C++ std::string
    std::string inputFilename(filename);

	// Find the position of the last dot in the filename, which represents the start of the file extension
    size_t dotPos = inputFilename.find_last_of('.');

	// Remove the file extension by taking the substring up to the dot position.
    output = inputFilename.substr(0, dotPos);

	// Append the new file extension ".ll" to the output filename
    output += fileExtension;
}

