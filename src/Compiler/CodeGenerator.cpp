#include "Compiler/CodeGenerator.hpp"

#include <sstream>

namespace Compiler {

std::string Instruction::toString() const {
    std::stringstream ss;
    
    switch (type) {
        case Type::LOAD:
            ss << "LOAD " << operand1 << ", " << operand2;
            break;
        case Type::STORE:
            ss << "STORE " << operand1 << ", " << operand2;
            break;
        case Type::MOVE:
            ss << "MOVE " << operand1 << ", " << operand2;
            break;
        case Type::ADD:
            ss << "ADD " << operand1 << ", " << operand2 << ", " << operand3;
            break;
        case Type::SUB:
            ss << "SUB " << operand1 << ", " << operand2 << ", " << operand3;
            break;
        case Type::MUL:
            ss << "MUL " << operand1 << ", " << operand2 << ", " << operand3;
            break;
        case Type::DIV:
            ss << "DIV " << operand1 << ", " << operand2 << ", " << operand3;
            break;
        case Type::CMP:
            ss << "CMP " << operand1 << ", " << operand2;
            break;
        case Type::JE:
            ss << "JE " << operand1;
            break;
        case Type::JNE:
            ss << "JNE " << operand1;
            break;
        case Type::JMP:
            ss << "JMP " << operand1;
            break;
        case Type::CALL:
            ss << "CALL " << operand1;
            break;
        case Type::RET:
            ss << "RET";
            break;
        case Type::PUSH:
            ss << "PUSH " << operand1;
            break;
        case Type::POP:
            ss << "POP " << operand1;
            break;
        case Type::NOP:
            ss << "NOP";
            break;
        case Type::LABEL:
            ss << operand1 << ":";
            break;
        case Type::COMMENT:
            ss << "# " << operand1;
            break;
        default:
            ss << "UNKNOWN";
            break;
    }
    
    if (!comment.empty()) {
        ss << " # " << comment;
    }
    
    return ss.str();
}

CodeGenerator::CodeGenerator() {
    // Initialize code generator
}

CodeGenerator::~CodeGenerator() = default;

std::vector<Instruction> CodeGenerator::generateOperation(const Operations::Operation& op) {
    std::vector<Instruction> instructions;
    
    // Add operation comment
    instructions.push_back(Instruction(Instruction::Type::COMMENT, 
                                     "Operation: " + op.typeToString() + " (" + op.targetName + ")"));
    
    switch (op.type) {
        case Operations::Type::Declaration:
            // Generate variable declaration
            instructions = generateDeclaration(op.targetName, Symbols::Variables::Type::STRING);
            break;
            
        case Operations::Type::Assignment:
            // Generate assignment
            {
                Symbols::ValuePtr value(std::string("placeholder"));
                auto assignInstr = generateAssignment(op.targetName, value);
                instructions.insert(instructions.end(), assignInstr.begin(), assignInstr.end());
            }
            break;
            
        case Operations::Type::FunctionCall:
            // Generate function call
            {
                std::vector<Symbols::ValuePtr> args;
                auto callInstr = generateFunctionCall(op.targetName, args);
                instructions.insert(instructions.end(), callInstr.begin(), callInstr.end());
            }
            break;
            
        default:
            // Unsupported operation
            instructions.push_back(Instruction(Instruction::Type::NOP, "", "", "Unsupported operation"));
            break;
    }
    
    return instructions;
}

std::vector<Instruction> CodeGenerator::generateDeclaration(const std::string& varName,
                                                          Symbols::Variables::Type type,
                                                          const Symbols::ValuePtr* initialValue) {
    std::vector<Instruction> instructions;
    
    // Allocate register or memory location for variable
    std::string location = getNextRegister();
    mapVariable(varName, location);
    
    instructions.push_back(Instruction(Instruction::Type::COMMENT, "Declare " + varName + " : " + typeToNative(type)));
    
    if (initialValue != nullptr) {
        // Generate code to load initial value
        auto loadInstr = generateLoadValue(*initialValue, location);
        instructions.insert(instructions.end(), loadInstr.begin(), loadInstr.end());
    } else {
        // Initialize with default value
        instructions.push_back(Instruction(Instruction::Type::LOAD, location, "0", "Default initialization"));
    }
    
    return instructions;
}

std::vector<Instruction> CodeGenerator::generateAssignment(const std::string& varName, 
                                                         const Symbols::ValuePtr& value) {
    std::vector<Instruction> instructions;
    
    std::string location = getVariableLocation(varName);
    if (location.empty()) {
        // Variable not declared, allocate new location
        location = getNextRegister();
        mapVariable(varName, location);
    }
    
    instructions.push_back(Instruction(Instruction::Type::COMMENT, "Assign to " + varName));
    
    // Generate code to load value
    auto loadInstr = generateLoadValue(value, location);
    instructions.insert(instructions.end(), loadInstr.begin(), loadInstr.end());
    
    return instructions;
}

std::vector<Instruction> CodeGenerator::generateFunctionCall(const std::string& functionName,
                                                           const std::vector<Symbols::ValuePtr>& args) {
    std::vector<Instruction> instructions;
    
    instructions.push_back(Instruction(Instruction::Type::COMMENT, "Call function " + functionName));
    
    // Push arguments onto stack (reverse order)
    for (int i = args.size() - 1; i >= 0; --i) {
        std::string tempReg = getNextRegister();
        auto loadInstr = generateLoadValue(args[i], tempReg);
        instructions.insert(instructions.end(), loadInstr.begin(), loadInstr.end());
        instructions.push_back(Instruction(Instruction::Type::PUSH, tempReg));
    }
    
    // Call function
    instructions.push_back(Instruction(Instruction::Type::CALL, functionName));
    
    // Clean up stack (remove arguments)
    for (size_t i = 0; i < args.size(); ++i) {
        std::string tempReg = getNextRegister();
        instructions.push_back(Instruction(Instruction::Type::POP, tempReg));
    }
    
    return instructions;
}

std::vector<Instruction> CodeGenerator::generateMethodCall(const std::string& objectName,
                                                         const std::string& methodName,
                                                         const std::vector<Symbols::ValuePtr>& args) {
    std::vector<Instruction> instructions;
    
    instructions.push_back(Instruction(Instruction::Type::COMMENT, 
                                     "Call method " + objectName + "." + methodName));
    
    // Load object reference
    std::string objectReg = getVariableLocation(objectName);
    if (objectReg.empty()) {
        objectReg = getNextRegister();
        instructions.push_back(Instruction(Instruction::Type::LOAD, objectReg, objectName));
    }
    
    // Push object as first argument (this pointer)
    instructions.push_back(Instruction(Instruction::Type::PUSH, objectReg));
    
    // Push other arguments
    for (int i = args.size() - 1; i >= 0; --i) {
        std::string tempReg = getNextRegister();
        auto loadInstr = generateLoadValue(args[i], tempReg);
        instructions.insert(instructions.end(), loadInstr.begin(), loadInstr.end());
        instructions.push_back(Instruction(Instruction::Type::PUSH, tempReg));
    }
    
    // Call method (using object class method table)
    std::string methodCall = objectName + "_" + methodName;
    instructions.push_back(Instruction(Instruction::Type::CALL, methodCall));
    
    // Clean up stack
    for (size_t i = 0; i < args.size() + 1; ++i) { // +1 for object reference
        std::string tempReg = getNextRegister();
        instructions.push_back(Instruction(Instruction::Type::POP, tempReg));
    }
    
    return instructions;
}

std::vector<Instruction> CodeGenerator::generateFunctionPrologue(const std::string& functionName) {
    std::vector<Instruction> instructions;
    
    currentFunction_ = functionName;
    
    // Function label
    instructions.push_back(Instruction(Instruction::Type::LABEL, functionName));
    instructions.push_back(Instruction(Instruction::Type::COMMENT, "Function prologue"));
    
    // Save frame pointer and set up new frame
    instructions.push_back(Instruction(Instruction::Type::PUSH, "rbp", "", "Save frame pointer"));
    instructions.push_back(Instruction(Instruction::Type::MOVE, "rbp", "rsp", "Set up new frame"));
    
    return instructions;
}

std::vector<Instruction> CodeGenerator::generateFunctionEpilogue() {
    std::vector<Instruction> instructions;
    
    instructions.push_back(Instruction(Instruction::Type::COMMENT, "Function epilogue"));
    
    // Restore frame pointer
    instructions.push_back(Instruction(Instruction::Type::MOVE, "rsp", "rbp", "Restore stack pointer"));
    instructions.push_back(Instruction(Instruction::Type::POP, "rbp", "", "Restore frame pointer"));
    
    // Return
    instructions.push_back(Instruction(Instruction::Type::RET));
    
    return instructions;
}

const std::vector<Instruction>& CodeGenerator::getInstructions() const {
    return instructions_;
}

void CodeGenerator::clear() {
    instructions_.clear();
    variableMap_.clear();
    labelMap_.clear();
    nextRegister_ = 0;
    nextLabel_ = 0;
}

std::vector<std::string> CodeGenerator::toAssembly() const {
    std::vector<std::string> assembly;
    
    for (const auto& instr : instructions_) {
        assembly.push_back(instr.toString());
    }
    
    return assembly;
}

std::string CodeGenerator::getNextRegister() {
    return "r" + std::to_string(nextRegister_++);
}

std::string CodeGenerator::getNextLabel() {
    return "L" + std::to_string(nextLabel_++);
}

void CodeGenerator::mapVariable(const std::string& varName, const std::string& location) {
    variableMap_[varName] = location;
}

std::string CodeGenerator::getVariableLocation(const std::string& varName) const {
    auto it = variableMap_.find(varName);
    if (it != variableMap_.end()) {
        return it->second;
    }
    return "";
}

void CodeGenerator::addInstruction(const Instruction& instruction) {
    instructions_.push_back(instruction);
}

void CodeGenerator::addInstructions(const std::vector<Instruction>& instructions) {
    instructions_.insert(instructions_.end(), instructions.begin(), instructions.end());
}

std::string CodeGenerator::typeToNative(Symbols::Variables::Type type) const {
    switch (type) {
        case Symbols::Variables::Type::INTEGER: return "int64";
        case Symbols::Variables::Type::DOUBLE:  return "float64";
        case Symbols::Variables::Type::STRING:  return "string";
        case Symbols::Variables::Type::BOOLEAN: return "bool";
        case Symbols::Variables::Type::OBJECT:  return "array";
        case Symbols::Variables::Type::CLASS:   return "object";
        default: return "unknown";
    }
}

std::vector<Instruction> CodeGenerator::generateLoadValue(const Symbols::ValuePtr& value,
                                                        const std::string& targetRegister) {
    std::vector<Instruction> instructions;
    
    if (value.getType() == Symbols::Variables::Type::NULL_TYPE) {
        instructions.push_back(Instruction(Instruction::Type::LOAD, targetRegister, "0", "Load null"));
        return instructions;
    }
    
    switch (value.getType()) {
        case Symbols::Variables::Type::INTEGER:
            instructions.push_back(Instruction(Instruction::Type::LOAD, targetRegister,
                                             std::to_string(value.get<int>()), "Load integer"));
            break;
            
        case Symbols::Variables::Type::DOUBLE:
            instructions.push_back(Instruction(Instruction::Type::LOAD, targetRegister,
                                             std::to_string(value.get<double>()), "Load double"));
            break;
            
        case Symbols::Variables::Type::STRING:
            instructions.push_back(Instruction(Instruction::Type::LOAD, targetRegister,
                                             "\"" + value.get<std::string>() + "\"", "Load string"));
            break;
            
        case Symbols::Variables::Type::BOOLEAN:
            instructions.push_back(Instruction(Instruction::Type::LOAD, targetRegister,
                                             value.get<bool>() ? "1" : "0", "Load boolean"));
            break;
            
        default:
            instructions.push_back(Instruction(Instruction::Type::LOAD, targetRegister, "0",
                                             "Load unsupported type"));
            break;
    }
    
    return instructions;
}

} // namespace Compiler