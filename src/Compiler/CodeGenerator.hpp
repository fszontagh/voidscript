#ifndef COMPILER_CODE_GENERATOR_HPP
#define COMPILER_CODE_GENERATOR_HPP

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "Interpreter/Operation.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

// Forward declaration
namespace Symbols {
    class ValuePtr;
}

namespace Compiler {

/**
 * @brief Native code instruction representation
 */
struct Instruction {
    enum class Type {
        // Memory operations
        LOAD,           // Load value into register
        STORE,          // Store register value to memory
        MOVE,           // Move value between registers
        
        // Arithmetic operations
        ADD,            // Addition
        SUB,            // Subtraction
        MUL,            // Multiplication
        DIV,            // Division
        MOD,            // Modulo
        
        // Comparison operations
        CMP,            // Compare
        JE,             // Jump if equal
        JNE,            // Jump if not equal
        JL,             // Jump if less
        JLE,            // Jump if less or equal
        JG,             // Jump if greater
        JGE,            // Jump if greater or equal
        
        // Control flow
        JMP,            // Unconditional jump
        CALL,           // Function call
        RET,            // Return from function
        PUSH,           // Push to stack
        POP,            // Pop from stack
        
        // Special
        NOP,            // No operation
        LABEL,          // Code label
        COMMENT         // Comment line
    };
    
    Type type;
    std::string operand1;
    std::string operand2;
    std::string operand3;
    std::string comment;
    
    Instruction(Type t, const std::string& op1 = "", const std::string& op2 = "", const std::string& op3 = "")
        : type(t), operand1(op1), operand2(op2), operand3(op3) {}
    
    std::string toString() const;
};

/**
 * @brief Converts operations to native code instructions
 * 
 * CodeGenerator is responsible for:
 * - Converting VoidScript operations to native assembly/intermediate code
 * - Managing registers and memory allocation
 * - Optimizing generated code
 * - Handling type conversions and runtime calls
 */
class CodeGenerator {
private:
    std::vector<Instruction> instructions_;
    std::unordered_map<std::string, std::string> variableMap_;  // Variable name -> register/memory location
    std::unordered_map<std::string, std::string> labelMap_;     // Label name -> actual label
    
    // Code generation state
    int nextRegister_ = 0;
    int nextLabel_ = 0;
    std::string currentFunction_;
    
public:
    /**
     * @brief Constructor
     */
    CodeGenerator();
    
    /**
     * @brief Destructor
     */
    ~CodeGenerator();
    
    /**
     * @brief Generate code for a single operation
     * @param op The operation to generate code for
     * @return Vector of generated instructions
     */
    std::vector<Instruction> generateOperation(const Operations::Operation& op);
    
    /**
     * @brief Generate code for variable declaration
     * @param varName Name of the variable
     * @param type Type of the variable
     * @param initialValue Initial value (optional)
     * @return Vector of generated instructions
     */
    std::vector<Instruction> generateDeclaration(const std::string& varName,
                                               Symbols::Variables::Type type,
                                               const Symbols::ValuePtr* initialValue = nullptr);
    
    /**
     * @brief Generate code for variable assignment
     * @param varName Name of the variable
     * @param value Value to assign
     * @return Vector of generated instructions
     */
    std::vector<Instruction> generateAssignment(const std::string& varName, 
                                              const Symbols::ValuePtr& value);
    
    /**
     * @brief Generate code for function call
     * @param functionName Name of the function
     * @param args Function arguments
     * @return Vector of generated instructions
     */
    std::vector<Instruction> generateFunctionCall(const std::string& functionName,
                                                const std::vector<Symbols::ValuePtr>& args);
    
    /**
     * @brief Generate code for method call
     * @param objectName Name of the object
     * @param methodName Name of the method
     * @param args Method arguments
     * @return Vector of generated instructions
     */
    std::vector<Instruction> generateMethodCall(const std::string& objectName,
                                              const std::string& methodName,
                                              const std::vector<Symbols::ValuePtr>& args);
    
    /**
     * @brief Generate function prologue
     * @param functionName Name of the function
     * @return Vector of generated instructions
     */
    std::vector<Instruction> generateFunctionPrologue(const std::string& functionName);
    
    /**
     * @brief Generate function epilogue
     * @return Vector of generated instructions
     */
    std::vector<Instruction> generateFunctionEpilogue();
    
    /**
     * @brief Get all generated instructions
     * @return Vector of all instructions
     */
    const std::vector<Instruction>& getInstructions() const;
    
    /**
     * @brief Clear all generated instructions
     */
    void clear();
    
    /**
     * @brief Convert instructions to assembly code strings
     * @return Vector of assembly code lines
     */
    std::vector<std::string> toAssembly() const;
    
    /**
     * @brief Get next available register name
     * @return Register name (e.g., "r0", "r1", etc.)
     */
    std::string getNextRegister();
    
    /**
     * @brief Get next available label name
     * @return Label name (e.g., "L0", "L1", etc.)
     */
    std::string getNextLabel();
    
    /**
     * @brief Map variable name to memory location/register
     * @param varName Variable name
     * @param location Memory location or register
     */
    void mapVariable(const std::string& varName, const std::string& location);
    
    /**
     * @brief Get memory location/register for variable
     * @param varName Variable name
     * @return Memory location or register, empty string if not found
     */
    std::string getVariableLocation(const std::string& varName) const;

private:
    /**
     * @brief Add instruction to the list
     * @param instruction Instruction to add
     */
    void addInstruction(const Instruction& instruction);
    
    /**
     * @brief Add multiple instructions to the list
     * @param instructions Instructions to add
     */
    void addInstructions(const std::vector<Instruction>& instructions);
    
    /**
     * @brief Convert VoidScript type to native type string
     * @param type VoidScript variable type
     * @return Native type string
     */
    std::string typeToNative(Symbols::Variables::Type type) const;
    
    /**
     * @brief Generate code to load a value into a register
     * @param value Value to load
     * @param targetRegister Target register
     * @return Vector of generated instructions
     */
    std::vector<Instruction> generateLoadValue(const Symbols::ValuePtr& value, 
                                             const std::string& targetRegister);
};

} // namespace Compiler

#endif // COMPILER_CODE_GENERATOR_HPP