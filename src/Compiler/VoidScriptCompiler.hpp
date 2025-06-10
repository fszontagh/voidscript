#ifndef VOIDSCRIPT_COMPILER_HPP
#define VOIDSCRIPT_COMPILER_HPP

#include <memory>
#include <string>
#include <vector>

#include "Compiler/CompilerBackend.hpp"

namespace Compiler {

/**
 * @brief Compilation options
 */
struct CompilationOptions {
    bool debug = false;
    bool optimize = false;
    bool generateAssembly = false;
    bool keepIntermediateFiles = false;
    std::string outputPath = "output.exe";
    std::string targetArchitecture = "x86_64";
    std::string compilerPath = "gcc";
    std::vector<std::string> includePaths;
    std::vector<std::string> libraryPaths;
    std::vector<std::string> libraries;
    
    CompilationOptions() = default;
};

/**
 * @brief Main compiler driver class
 * 
 * VoidScriptCompiler is the main entry point for compilation and provides:
 * - High-level compilation interface
 * - Integration with existing Parser and OperationsFactory
 * - Command-line interface support
 * - Build system integration
 */
class VoidScriptCompiler {
private:
    CompilationOptions options_;
    std::unique_ptr<CompilerBackend> backend_;
    
    // Compilation state
    std::string sourceFile_;
    std::vector<std::string> intermediateFiles_;
    bool isInitialized_ = false;
    
public:
    /**
     * @brief Constructor
     * @param options Compilation options
     */
    VoidScriptCompiler(const CompilationOptions& options = CompilationOptions{});
    
    /**
     * @brief Destructor
     */
    ~VoidScriptCompiler();
    
    /**
     * @brief Initialize the compiler with the given options
     * @return true if initialization was successful
     */
    bool initialize();
    
    /**
     * @brief Compile a VoidScript source file
     * @param sourceFile Path to the source file
     * @return true if compilation was successful
     */
    bool compileFile(const std::string& sourceFile);
    
    /**
     * @brief Compile VoidScript source code from string
     * @param sourceCode Source code to compile
     * @param filename Virtual filename for error reporting
     * @return true if compilation was successful
     */
    bool compileSource(const std::string& sourceCode, const std::string& filename = "<string>");
    
    /**
     * @brief Compile operations that are already in the operations container
     * This method integrates with the existing parser workflow
     * @return true if compilation was successful
     */
    bool compileOperations();
    
    /**
     * @brief Get compilation options
     * @return Current compilation options
     */
    const CompilationOptions& getOptions() const;
    
    /**
     * @brief Set compilation options
     * @param options New compilation options
     */
    void setOptions(const CompilationOptions& options);
    
    /**
     * @brief Get the generated assembly code
     * @return Vector of assembly code lines
     */
    std::vector<std::string> getAssemblyCode() const;
    
    /**
     * @brief Get the path of the compiled binary
     * @return Path to the compiled binary
     */
    std::string getOutputPath() const;
    
    /**
     * @brief Get compilation errors and warnings
     * @return Vector of error/warning messages
     */
    std::vector<std::string> getMessages() const;
    
    /**
     * @brief Check if compilation was successful
     * @return true if compilation was successful
     */
    bool isSuccessful() const;
    
    /**
     * @brief Clean up intermediate files
     */
    void cleanup();
    
    /**
     * @brief Create default compilation options for development
     * @return Development compilation options
     */
    static CompilationOptions createDebugOptions();
    
    /**
     * @brief Create default compilation options for release
     * @return Release compilation options
     */
    static CompilationOptions createReleaseOptions();
    
    /**
     * @brief Get version information
     * @return Compiler version string
     */
    static std::string getVersion();

private:
    /**
     * @brief Validate compilation options
     * @return true if options are valid
     */
    bool validateOptions();
    
    /**
     * @brief Generate assembly file from compiled operations
     * @param assemblyPath Path for the assembly file
     * @return true if successful
     */
    bool generateAssemblyFile(const std::string& assemblyPath);
    
    /**
     * @brief Compile C source to binary using external compiler
     * @param sourcePath Path to the C source file
     * @param binaryPath Path for the output binary
     * @return true if successful
     */
    bool compileToBinary(const std::string& sourcePath, const std::string& binaryPath);
    
    /**
     * @brief Add intermediate file to cleanup list
     * @param filePath Path to intermediate file
     */
    void addIntermediateFile(const std::string& filePath);
    
    /**
     * @brief Log compilation message
     * @param message Message to log
     * @param isError true if this is an error message
     */
    void logMessage(const std::string& message, bool isError = false);
    
    // Error and message tracking
    std::vector<std::string> messages_;
    bool compilationSuccessful_ = false;
};

} // namespace Compiler

#endif // VOIDSCRIPT_COMPILER_HPP