#include "Compiler/VoidScriptCompiler.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

// VoidScript core includes for parsing
#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/OperationsFactory.hpp"

namespace Compiler {

VoidScriptCompiler::VoidScriptCompiler(const CompilationOptions& options)
    : options_(options) {
    // Constructor - initialization will be done in initialize()
}

VoidScriptCompiler::~VoidScriptCompiler() {
    cleanup();
}

bool VoidScriptCompiler::initialize() {
    if (isInitialized_) {
        return true;
    }
    
    try {
        // Validate compilation options
        if (!validateOptions()) {
            logMessage("Invalid compilation options", true);
            return false;
        }
        
        // Create compiler backend
        backend_ = std::make_unique<CompilerBackend>(options_.debug, options_.outputPath);
        
        isInitialized_ = true;
        logMessage("VoidScript compiler initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        logMessage("Failed to initialize compiler: " + std::string(e.what()), true);
        return false;
    }
}

bool VoidScriptCompiler::compileFile(const std::string& sourceFile) {
    if (!initialize()) {
        return false;
    }
    
    sourceFile_ = sourceFile;
    logMessage("Compiling file: " + sourceFile);
    
    try {
        // Read source file
        std::ifstream file(sourceFile);
        if (!file.is_open()) {
            logMessage("Failed to open source file: " + sourceFile, true);
            return false;
        }
        
        // Read file content
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string sourceCode = buffer.str();
        file.close();
        
        // Compile the source code
        return compileSource(sourceCode, sourceFile);
        
    } catch (const std::exception& e) {
        logMessage("Error reading source file: " + std::string(e.what()), true);
        return false;
    }
}

bool VoidScriptCompiler::compileSource(const std::string& sourceCode, const std::string& filename) {
    if (!initialize()) {
        return false;
    }
    
    logMessage("Compiling source code from: " + filename);
    
    try {
        // Initialize SymbolContainer with the script name
        Symbols::SymbolContainer::initialize(filename);
        logMessage("SymbolContainer initialized with script: " + filename);
        
        // Get SymbolContainer instance and create scope for the file
        auto symbolContainer = Symbols::SymbolContainer::instance();
        symbolContainer->create(filename);
        const std::string ns = symbolContainer->currentScopeName();
        logMessage("Created compilation scope: " + ns);
        
        // Initialize lexer and parser (following VoidScript.hpp pattern)
        auto lexer = std::make_shared<Lexer::Lexer>();
        auto parser = std::make_shared<Parser::Parser>();
        
        // Set up lexer with keywords
        lexer->setKeyWords(Parser::Parser::keywords);
        
        // Add source input to lexer
        lexer->addNamespaceInput(ns, sourceCode);
        logMessage("Added source code to lexer namespace: " + ns);
        
        // Tokenize the source code
        const auto tokens = lexer->tokenizeNamespace(ns);
        logMessage("Tokenized source: " + std::to_string(tokens.size()) + " tokens");
        
        if (options_.debug) {
            logMessage("Debug: Tokens for namespace '" + ns + "':");
            for (const auto& token : tokens) {
                logMessage("  " + token.dump());
            }
        }
        
        // Parse the script and populate Operations::Container
        parser->parseScript(tokens, sourceCode, filename);
        
        // Get the operations that were generated
        auto operations = Operations::Container::instance()->getAll(ns);
        logMessage("Generated " + std::to_string(operations.size()) + " operations from parsing");
        
        if (options_.debug) {
            logMessage("Debug: Operations for namespace '" + ns + "':");
            for (const auto& op : operations) {
                logMessage("  " + op->toString());
            }
        }
        
        // Now compile the operations using the backend
        return compileOperations();
        
    } catch (const std::exception& e) {
        logMessage("Error compiling source: " + std::string(e.what()), true);
        return false;
    }
}

bool VoidScriptCompiler::compileOperations() {
    if (!initialize()) {
        return false;
    }
    
    logMessage("Compiling operations from container");
    
    try {
        // Compile operations using the backend
        backend_->compile();
        
        // Generate assembly file
        if (!backend_->generateBinary()) {
            logMessage("Failed to generate assembly output", true);
            compilationSuccessful_ = false;
            return false;
        }
        
        // If we only want assembly/source, stop here
        if (options_.generateAssembly) {
            logMessage("Source code generation completed successfully");
            compilationSuccessful_ = true;
            return true;
        }
        
        // Generate the final binary from C source
        std::string sourcePath = options_.outputPath + ".c";
        std::string binaryPath = options_.outputPath;
        
        addIntermediateFile(sourcePath);
        
        if (!compileToBinary(sourcePath, binaryPath)) {
            logMessage("Failed to compile binary from source file", true);
            compilationSuccessful_ = false;
            return false;
        }
        
        logMessage("Binary compilation completed successfully");
        compilationSuccessful_ = true;
        return true;
        
    } catch (const Exception& e) {
        logMessage("Compilation error: " + std::string(e.what()), true);
        compilationSuccessful_ = false;
        return false;
    } catch (const std::exception& e) {
        logMessage("Unexpected error during compilation: " + std::string(e.what()), true);
        compilationSuccessful_ = false;
        return false;
    }
}

const CompilationOptions& VoidScriptCompiler::getOptions() const {
    return options_;
}

void VoidScriptCompiler::setOptions(const CompilationOptions& options) {
    options_ = options;
    isInitialized_ = false; // Force re-initialization with new options
}

std::vector<std::string> VoidScriptCompiler::getAssemblyCode() const {
    if (backend_) {
        return backend_->getGeneratedCode();
    }
    return {};
}

std::string VoidScriptCompiler::getOutputPath() const {
    if (backend_) {
        return backend_->getOutputPath();
    }
    return options_.outputPath;
}

std::vector<std::string> VoidScriptCompiler::getMessages() const {
    return messages_;
}

bool VoidScriptCompiler::isSuccessful() const {
    return compilationSuccessful_;
}

void VoidScriptCompiler::cleanup() {
    if (!options_.keepIntermediateFiles) {
        // Remove intermediate files
        for (const auto& file : intermediateFiles_) {
            try {
                std::remove(file.c_str());
                logMessage("Removed intermediate file: " + file);
            } catch (const std::exception& e) {
                logMessage("Failed to remove intermediate file " + file + ": " + e.what());
            }
        }
        intermediateFiles_.clear();
    }
}

CompilationOptions VoidScriptCompiler::createDebugOptions() {
    CompilationOptions options;
    options.debug = true;
    options.optimize = false;
    options.generateAssembly = true;
    options.keepIntermediateFiles = true;
    options.outputPath = "debug_output";
    return options;
}

CompilationOptions VoidScriptCompiler::createReleaseOptions() {
    CompilationOptions options;
    options.debug = false;
    options.optimize = true;
    options.generateAssembly = false;
    options.keepIntermediateFiles = false;
    options.outputPath = "release_output";
    return options;
}

std::string VoidScriptCompiler::getVersion() {
    return "VoidScript Compiler v1.0.0";
}

bool VoidScriptCompiler::validateOptions() {
    // Check output path
    if (options_.outputPath.empty()) {
        logMessage("Output path cannot be empty", true);
        return false;
    }
    
    // Check target architecture
    if (options_.targetArchitecture != "x86_64" && 
        options_.targetArchitecture != "x86" && 
        options_.targetArchitecture != "arm64" &&
        options_.targetArchitecture != "arm") {
        logMessage("Unsupported target architecture: " + options_.targetArchitecture, true);
        return false;
    }
    
    // Check compiler path
    if (options_.compilerPath.empty()) {
        logMessage("Compiler path cannot be empty", true);
        return false;
    }
    
    return true;
}

bool VoidScriptCompiler::generateAssemblyFile(const std::string& assemblyPath) {
    if (!backend_) {
        logMessage("Backend not initialized", true);
        return false;
    }
    
    try {
        std::ofstream asmFile(assemblyPath);
        if (!asmFile.is_open()) {
            logMessage("Failed to create assembly file: " + assemblyPath, true);
            return false;
        }
        
        // Write assembly code
        auto assemblyCode = backend_->getGeneratedCode();
        for (const auto& line : assemblyCode) {
            asmFile << line << "\n";
        }
        
        asmFile.close();
        addIntermediateFile(assemblyPath);
        logMessage("Assembly file generated: " + assemblyPath);
        return true;
        
    } catch (const std::exception& e) {
        logMessage("Error generating assembly file: " + std::string(e.what()), true);
        return false;
    }
}

bool VoidScriptCompiler::compileToBinary(const std::string& sourcePath, const std::string& binaryPath) {
    logMessage("Compiling to binary: " + sourcePath + " -> " + binaryPath);
    
    try {
        // Check if compiler exists
        std::string checkCommand = "which " + options_.compilerPath + " > /dev/null 2>&1";
        if (std::system(checkCommand.c_str()) != 0) {
            logMessage("System compiler '" + options_.compilerPath + "' not found", true);
            logMessage("Please install " + options_.compilerPath + " or specify a different compiler with --compiler", true);
            return false;
        }
        
        // Build compiler command
        std::string command = options_.compilerPath;
        
        // Add target architecture flags
        if (options_.targetArchitecture == "x86_64") {
            command += " -m64";
        } else if (options_.targetArchitecture == "i386") {
            command += " -m32";
        } else if (options_.targetArchitecture == "arm64") {
            command += " -march=armv8-a";
        }
        
        // Add output file
        command += " -o \"" + binaryPath + "\"";
        command += " \"" + sourcePath + "\"";
        
        // Add include paths
        for (const auto& incPath : options_.includePaths) {
            command += " -I\"" + incPath + "\"";
        }
        
        // Add library paths
        for (const auto& libPath : options_.libraryPaths) {
            command += " -L\"" + libPath + "\"";
        }
        
        // Add libraries
        for (const auto& lib : options_.libraries) {
            command += " -l" + lib;
        }
        
        // Add standard C libraries (needed for printf, malloc, etc.)
        command += " -lc";
        
        // Add optimization flags
        if (options_.optimize) {
            command += " -O2";
        } else {
            command += " -O0";  // No optimization for debug builds
        }
        
        // Add debug flags
        if (options_.debug) {
            command += " -g -DDEBUG";
        }
        
        // Add other useful flags
        command += " -Wall -Wextra";  // Enable warnings
        
        logMessage("Executing compiler command:");
        logMessage("  " + command);
        
        // Execute compiler command
        int result = std::system(command.c_str());
        if (result != 0) {
            logMessage("Source compilation failed with exit code: " + std::to_string(result), true);
            logMessage("This usually indicates syntax errors in the generated source code or missing dependencies", true);
            return false;
        }
        
        // Check if binary was actually created
        std::ifstream binaryFile(binaryPath);
        if (!binaryFile.good()) {
            logMessage("Binary file was not created: " + binaryPath, true);
            return false;
        }
        binaryFile.close();
        
        logMessage("Binary compiled successfully: " + binaryPath);
        
        // Make the binary executable on Unix systems
#ifdef __unix__
        std::string chmodCommand = "chmod +x \"" + binaryPath + "\"";
        std::system(chmodCommand.c_str());
        logMessage("Made binary executable");
#endif
        
        return true;
        
    } catch (const std::exception& e) {
        logMessage("Error during binary compilation: " + std::string(e.what()), true);
        return false;
    }
}

void VoidScriptCompiler::addIntermediateFile(const std::string& filePath) {
    intermediateFiles_.push_back(filePath);
}

void VoidScriptCompiler::logMessage(const std::string& message, bool isError) {
    std::string prefix = isError ? "[ERROR] " : "[INFO] ";
    std::string fullMessage = prefix + message;
    
    messages_.push_back(fullMessage);
    
    if (options_.debug) {
        if (isError) {
            std::cerr << fullMessage << std::endl;
        } else {
            std::cout << fullMessage << std::endl;
        }
    }
}

} // namespace Compiler