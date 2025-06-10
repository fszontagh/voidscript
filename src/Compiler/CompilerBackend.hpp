#ifndef COMPILER_BACKEND_HPP
#define COMPILER_BACKEND_HPP

#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <set>

#include "Interpreter/Operation.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/Nodes/Statement/ConditionalStatementNode.hpp"
#include "Interpreter/Nodes/Statement/WhileStatementNode.hpp"
#include "Interpreter/Nodes/Statement/CStyleForStatementNode.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Compiler/CodeGenerator.hpp"
#include "Compiler/RuntimeLibrary.hpp"

namespace Compiler {

/**
 * @brief Exception class for compiler-related errors
 */
class Exception : public std::exception {
private:
    std::string message_;

public:
    Exception(const std::string& msg) : message_(msg) {}
    
    virtual const char* what() const noexcept override {
        return message_.c_str();
    }
};

/**
 * @brief Main compiler coordinator that processes operations queue
 * 
 * CompilerBackend is responsible for:
 * - Processing the operations queue (similar to how Interpreter::run works)
 * - Coordinating with CodeGenerator to convert operations to native code
 * - Managing compilation context and state
 * - Producing the final compiled output
 */
class CompilerBackend {
private:
    bool debug_ = false;
    std::unique_ptr<CodeGenerator> codeGenerator_;
    std::unique_ptr<RuntimeLibrary> runtimeLibrary_;
    std::unique_ptr<Interpreter::Interpreter> interpreter_;
    std::string outputPath_;
    
    // Compilation state
    std::string currentNamespace_;
    std::vector<std::string> generatedCode_;
    
    // Track declared variables to distinguish initial assignments from reassignments
    std::set<std::string> declaredVariables_;
    
public:
    /**
     * @brief Construct compiler backend with optional debug output
     * @param debug Enable compiler debug output
     * @param outputPath Path for the compiled output
     */
    CompilerBackend(bool debug = false, const std::string& outputPath = "output.exe");
    
    /**
     * @brief Destructor
     */
    ~CompilerBackend();
    
    /**
     * @brief Compile all operations in the current namespace
     * Similar to Interpreter::run() but generates code instead of executing
     */
    void compile();
    
    /**
     * @brief Compile operations in a specific namespace
     * @param ns The namespace to compile
     */
    void compileNamespace(const std::string& ns);
    
    /**
     * @brief Compile a single operation
     * @param op The operation to compile
     * @throws Compiler::Exception if compilation fails
     */
    void compileOperation(const Operations::Operation& op);
    
    /**
     * @brief Generate the final binary output
     * @return true if successful, false otherwise
     */
    bool generateBinary();
    
    /**
     * @brief Get the generated assembly/intermediate code
     * @return Vector of generated code lines
     */
    const std::vector<std::string>& getGeneratedCode() const;
    
    /**
     * @brief Set the output path for the compiled binary
     * @param path The output file path
     */
    void setOutputPath(const std::string& path);
    
    /**
     * @brief Get the current output path
     * @return The output file path
     */
    const std::string& getOutputPath() const;
    
    /**
     * @brief Enable or disable debug output
     * @param debug Debug flag
     */
    void setDebug(bool debug);
    
    /**
     * @brief Check if debug mode is enabled
     * @return true if debug mode is enabled
     */
    bool isDebugEnabled() const;

private:
    /**
     * @brief Initialize the code generator and runtime library
     */
    void initialize();
    
    /**
     * @brief Add generated code line
     * @param code The code line to add
     */
    void addCodeLine(const std::string& code);
    
    /**
     * @brief Convert pseudo-assembly instruction to C code
     * @param instruction The instruction to convert
     * @return C code equivalent or empty string if not convertible
     */
    std::string convertInstructionToC(const std::string& instruction);
    
    /**
     * @brief Generate C code for printnl function calls with actual arguments
     * @param op The operation containing the printnl call
     * @return Generated C code for the printnl call
     */
    std::string generatePrintnlCall(const Operations::Operation& op);
    
    /**
     * @brief Extract argument value from ExpressionNode for compilation
     * @param expr The expression node to extract value from
     * @return C code representation of the argument value
     */
    std::string extractArgumentValue(const Interpreter::ExpressionNode& expr);
    
    /**
     * @brief Evaluate expression using VoidScript interpreter to get actual value
     * @param expr The expression node to evaluate
     * @return The evaluated ValuePtr
     */
    Symbols::ValuePtr evaluateExpression(const Interpreter::ExpressionNode& expr);
    
    /**
     * @brief Convert a ValuePtr to appropriate C code representation
     * @param value The value to convert
     * @return C code string representation of the value
     */
    std::string valuePtrToCCode(const Symbols::ValuePtr& value);
    
    /**
     * @brief Generate C code for variable assignment operations
     * @param op The assignment operation to process
     * @return Generated C code for the assignment
     */
    std::string generateAssignmentCall(const Operations::Operation& op);
    
    /**
     * @brief Generate C code for array assignment operations
     * @param op The array assignment operation to process
     * @return Generated C code for the array assignment
     */
    std::string generateArrayAssignment(const Operations::Operation& op);
    
    /**
     * @brief Generate C code for function call operations
     * @param op The function call operation to process
     * @return Generated C code for the function call
     */
    std::string generateFunctionCall(const Operations::Operation& op);
    
    /**
     * @brief Generate C function definition from function declaration operation
     * @param op The function declaration operation to process
     * @return Generated C function definition
     */
    std::string generateFunctionDefinition(const Operations::Operation& op);
    
    /**
     * @brief Generate C code for if/else conditional statements
     * @param op The conditional operation to process
     * @return Generated C code for the if/else statement
     */
    std::string generateIfStatement(const Operations::Operation& op);
    
    /**
     * @brief Generate C code for while loop statements
     * @param op The while loop operation to process
     * @return Generated C code for the while loop
     */
    std::string generateWhileLoop(const Operations::Operation& op);
    
    /**
     * @brief Generate C code for for loop statements
     * @param op The for loop operation to process
     * @return Generated C code for the for loop
     */
    std::string generateForLoop(const Operations::Operation& op);
    
    /**
     * @brief Generate C code for foreach loop statements
     * @param op The foreach loop operation to process
     * @return Generated C code for the foreach loop
     */
    std::string generateForeachLoop(const Operations::Operation& op);
    
    /**
     * @brief Generate C code for expression evaluation (conditions, etc.)
     * @param expr The expression node to evaluate
     * @return Generated C code for the expression
     */
    std::string generateExpressionCode(const Interpreter::ExpressionNode& expr);
    
    /**
     * @brief Generate C code for a block of statements
     * @param statements Vector of statement nodes to compile
     * @return Generated C code for the statement block
     */
    std::string generateStatementBlock(const std::vector<std::unique_ptr<Interpreter::StatementNode>>& statements);
    
    /**
     * @brief Log debug information
     * @param message Debug message
     */
    void debugLog(const std::string& message);
};

} // namespace Compiler

#endif // COMPILER_BACKEND_HPP