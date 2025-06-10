#include <unistd.h>  // for isatty, STDIN_FILENO

#include <iostream>
#include <unordered_map>
#include <vector>
#include <filesystem>

#include "options.h"
#include "utils.h"
#include "Compiler/VoidScriptCompiler.hpp"

using namespace Compiler;

// Supported command-line parameters and descriptions
const std::unordered_map<std::string, std::string> params = {
    { "--help",                  "Print this help message"                                                        },
    { "--version",               "Print the version of the program"                                               },
    { "--debug",                 "Enable debug compilation (includes debug symbols)"                             },
    { "--optimize",              "Enable optimization (O2 level)"                                                },
    { "--output",                "Specify output file path (default: output.exe)"                               },
    { "--target",                "Target architecture (x86_64, i386, arm64)"                                    },
    { "--keep-intermediate",     "Keep intermediate assembly files"                                              },
    { "--generate-assembly",     "Generate assembly file only (don't compile to binary)"                        },
    { "--compiler",              "Specify C compiler to use (default: gcc)"                                     },
    { "--include",               "Add include directory"                                                         },
    { "--library-path",          "Add library search path"                                                       },
    { "--library",               "Link with library"                                                             },
};

void printUsage(const std::string& programName) {
    std::cout << "Usage: " << programName;
    for (const auto& [key, value] : params) {
        std::cout << " [" << key << "]";
    }
    std::cout << " <input.vs>\n\n";
    
    std::cout << "VoidScript Compiler - Compile VoidScript source files to native executables\n\n";
    
    std::cout << "Options:\n";
    for (const auto& [key, value] : params) {
        std::cout << "  " << key << ": " << value << "\n";
    }
    
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " script.vs\n";
    std::cout << "  " << programName << " --debug --output myapp script.vs\n";
    std::cout << "  " << programName << " --optimize --target x86_64 script.vs\n";
    std::cout << "  " << programName << " --generate-assembly script.vs\n";
}

int main(int argc, char* argv[]) {
    std::string programName = std::filesystem::path(argv[0]).filename().string();
    
    if (argc < 2) {
        printUsage(programName);
        return 1;
    }
    
    // Parse command line arguments
    CompilationOptions options;
    std::string inputFile;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            printUsage(programName);
            return 0;
        }
        else if (arg == "--version") {
            std::cout << "VoidScript Compiler\n";
            std::cout << "Version:      " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH << " ("
                      << VERSION_GIT_HASH << ")\n";
            std::cout << "Architecture: " << VERSION_ARCH << "\n";
            std::cout << "System:       " << VERSION_SYSTEM_NAME << "\n";
            std::cout << "Compiler:     " << VoidScriptCompiler::getVersion() << "\n";
            return 0;
        }
        else if (arg == "--debug") {
            options.debug = true;
        }
        else if (arg == "--optimize") {
            options.optimize = true;
        }
        else if (arg == "--keep-intermediate") {
            options.keepIntermediateFiles = true;
        }
        else if (arg == "--generate-assembly") {
            options.generateAssembly = true;
        }
        else if (arg == "--output" && i + 1 < argc) {
            options.outputPath = argv[++i];
        }
        else if (arg == "--target" && i + 1 < argc) {
            options.targetArchitecture = argv[++i];
            // Validate target architecture
            if (options.targetArchitecture != "x86_64" && 
                options.targetArchitecture != "i386" && 
                options.targetArchitecture != "arm64") {
                std::cerr << "Error: Unsupported target architecture '" << options.targetArchitecture << "'\n";
                std::cerr << "Supported architectures: x86_64, i386, arm64\n";
                return 1;
            }
        }
        else if (arg == "--compiler" && i + 1 < argc) {
            options.compilerPath = argv[++i];
        }
        else if (arg == "--include" && i + 1 < argc) {
            options.includePaths.push_back(argv[++i]);
        }
        else if (arg == "--library-path" && i + 1 < argc) {
            options.libraryPaths.push_back(argv[++i]);
        }
        else if (arg == "--library" && i + 1 < argc) {
            options.libraries.push_back(argv[++i]);
        }
        else if (arg.rfind("-", 0) == 0) {
            std::cerr << "Error: Unknown option '" << arg << "'\n";
            printUsage(programName);
            return 1;
        }
        else if (inputFile.empty()) {
            inputFile = arg;
        }
        else {
            std::cerr << "Error: Multiple input files specified. Only one input file is supported.\n";
            printUsage(programName);
            return 1;
        }
    }
    
    // Validate input file
    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified.\n";
        printUsage(programName);
        return 1;
    }
    
    if (!utils::exists(inputFile)) {
        std::cerr << "Error: Input file '" << inputFile << "' does not exist.\n";
        return 1;
    }
    
    // Check if input file has .vs extension
    std::filesystem::path inputPath(inputFile);
    if (inputPath.extension() != ".vs") {
        std::cerr << "Warning: Input file does not have .vs extension.\n";
    }
    
    // Set default output filename if not specified
    if (options.outputPath == "output.exe") {
        std::filesystem::path outputPath = inputPath;
        outputPath.replace_extension("");
        if (outputPath.empty()) {
            outputPath = "a.out";
        }
        options.outputPath = outputPath.string();
    }
    
    // Create and initialize the compiler
    VoidScriptCompiler compiler(options);
    
    if (!compiler.initialize()) {
        std::cerr << "Error: Failed to initialize compiler.\n";
        for (const auto& message : compiler.getMessages()) {
            std::cerr << message << "\n";
        }
        return 1;
    }
    
    // Compile the input file
    std::cout << "Compiling '" << inputFile << "'...\n";
    
    bool success = compiler.compileFile(inputFile);
    
    // Display compilation messages
    for (const auto& message : compiler.getMessages()) {
        if (message.find("Error:") != std::string::npos || 
            message.find("error:") != std::string::npos) {
            std::cerr << message << "\n";
        } else {
            std::cout << message << "\n";
        }
    }
    
    if (success) {
        if (options.generateAssembly) {
            std::cout << "Assembly generation completed successfully.\n";
        } else {
            std::cout << "Compilation completed successfully.\n";
            std::cout << "Output: " << compiler.getOutputPath() << "\n";
        }
        
        // Clean up intermediate files unless requested to keep them
        if (!options.keepIntermediateFiles) {
            compiler.cleanup();
        }
        
        return 0;
    } else {
        std::cerr << "Compilation failed.\n";
        return 1;
    }
}