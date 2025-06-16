#ifndef INTERPRETER_DEFINE_VARIABLE_STATEMENT_NODE_HPP
#define INTERPRETER_DEFINE_VARIABLE_STATEMENT_NODE_HPP

#include <memory>
#include <string>
#include <utility>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"

namespace Interpreter {

class DeclareVariableStatementNode : public StatementNode {
    std::string                     variableName_;
    Symbols::Variables::Type        variableType_;
    std::unique_ptr<ExpressionNode> expression_;
    std::string                     ns;
    bool                            isConst_;


  public:
    // isConst: if true, declares a constant; otherwise a mutable variable
    DeclareVariableStatementNode(std::string name, const std::string & ns, Symbols::Variables::Type type,
                                 std::unique_ptr<ExpressionNode> expr, const std::string & file_name, int file_line,
                                 size_t line_column, bool isConst = false) :
        StatementNode(file_name, file_line, line_column),
        variableName_(std::move(name)),
        variableType_(type),
        expression_(std::move(expr)),
        ns(ns),
        isConst_(isConst) {}

    void interpret(Interpreter & interpreter) const override {
        try {
            Symbols::ValuePtr initValue;
            if (expression_) { // Changed from initializerExpr_ to expression_
                initValue = expression_->evaluate(interpreter);
            } else {
                // Default initialize based on type_ if no initializer
                initValue = Symbols::ValuePtr::null(variableType_); // Changed from type_ to variableType_
            }

            auto value = initValue; // Use initValue for further processing

            auto * sc = Symbols::SymbolContainer::instance();

            // ALWAYS use the current runtime scope for variable declarations
            // This ensures that variables declared inside functions go to the function's runtime scope,
            // not the parse-time scope stored in ns member variable
            std::string target_scope_name = sc->currentScopeName();
            auto        targetTable       = sc->getScopeTable(target_scope_name);
            
            // DEBUG: Print scope information
            std::cerr << "DeclareVariableStatementNode: declaring variable '" << variableName_
                      << "' in scope '" << target_scope_name << "' (parse-time ns was: '" << ns << "')" << std::endl;

            if (!targetTable) {
                // This should ideally not happen if parser/caller creates scopes correctly
                throw Exception("Target scope '" + target_scope_name +
                                    "' for variable declaration does not exist",
                                filename_, line_, column_);
            }

            // Check if variable/constant already exists in the target scope's table
            auto existing_var = targetTable->get(Symbols::SymbolContainer::DEFAULT_VARIABLES_SCOPE, variableName_);
            if (existing_var) {
                // If we're in a loop scope (current runtime scope name contains "for_" or "while_"), allow redeclaration
                if (target_scope_name.find("for_") != std::string::npos ||
                    target_scope_name.find("while_") != std::string::npos) {
                    // Update the existing variable's value
                    existing_var->setValue(value);
                    return;
                }
                throw Exception(
                    "Variable '" + variableName_ + "' already declared in scope '" + target_scope_name + "'",
                    filename_, line_, column_);
            }

            Symbols::SymbolPtr existing_const_check =
                targetTable->get(Symbols::SymbolContainer::DEFAULT_CONSTANTS_SCOPE, variableName_);
            if (existing_const_check) {
                throw Exception(
                    "Cannot redefine constant '" + variableName_ + "' in scope '" + target_scope_name + "'",
                    filename_, line_, column_);
            }

            if (value == Symbols::Variables::Type::NULL_TYPE) {
                value.setType(variableType_);
            }

            // For class types, we need to handle the comparison differently
            if (variableType_ == Symbols::Variables::Type::CLASS) {
                // The value should be either CLASS type or OBJECT type (from new ClassName())
                if (value.getType() != Symbols::Variables::Type::CLASS &&
                    value.getType() != Symbols::Variables::Type::OBJECT &&
                    value.getType() != Symbols::Variables::Type::NULL_TYPE) {
                    std::string expected = Symbols::Variables::TypeToString(variableType_);
                    std::string actual = Symbols::Variables::TypeToString(value.getType());
                    throw Exception("Type mismatch for variable '" + variableName_ + "': expected '" + expected +
                                        "' but got '" + actual + "' in scope '" + target_scope_name + "'",
                                    filename_, line_, column_);
                }
                
                // If it's an OBJECT from a 'new' expression, ensure we set the type to CLASS
                if (value.getType() == Symbols::Variables::Type::OBJECT) {
                    auto& objMap = value.get<Symbols::ObjectMap>();
                    
                    // Check for various class name fields
                    auto it = objMap.find("__class__");
                    if (it == objMap.end()) {
                        it = objMap.find("$class_name");
                    }
                    
                    if (it != objMap.end() && it->second->getType() == Symbols::Variables::Type::STRING) {
                        // This is actually a class instance, but we need to create a new ValuePtr using makeClassInstance
                        // Create a properly typed class instance
                        value = Symbols::ValuePtr::makeClassInstance(objMap);
                    }
                } else if (value.getType() == Symbols::Variables::Type::NULL_TYPE) {
                    // For CLASS variables initialized with NULL, create a simple null value
                    // DO NOT call ValuePtr::null(CLASS) or makeClassInstance as it can cause infinite loops
                    // Just manually set the type to CLASS on the existing null value
                    value.setType(Symbols::Variables::Type::CLASS);
                }
            } else if (variableType_ == Symbols::Variables::Type::ENUM) {
                // For enum types, allow integer values to be assigned (since enum values are integers)
                if (value.getType() != Symbols::Variables::Type::ENUM &&
                    value.getType() != Symbols::Variables::Type::INTEGER &&
                    value.getType() != Symbols::Variables::Type::NULL_TYPE) {
                    std::string expected = Symbols::Variables::TypeToString(variableType_);
                    std::string actual = Symbols::Variables::TypeToString(value.getType());
                    throw Exception("Type mismatch for variable '" + variableName_ + "': expected '" + expected +
                                        "' but got '" + actual + "' in scope '" + target_scope_name + "'",
                                    filename_, line_, column_);
                }
                // Don't try to convert types - just allow the assignment as-is
                // Enum variables can store integer values since enums are internally integers
            } else if (value.getType() != variableType_) {
                std::string expected = Symbols::Variables::TypeToString(variableType_);
                std::string actual = Symbols::Variables::TypeToString(value.getType());
                throw Exception("Type mismatch for variable '" + variableName_ + "': expected '" + expected +
                                    "' but got '" + actual + "' in scope '" + target_scope_name + "'",
                                filename_, line_, column_);
            }


            // Create a constant or variable symbol
            // The symbol's own context should be the target scope (ns)
            std::shared_ptr<Symbols::Symbol> symbol_to_define;
            if (isConst_) {
                symbol_to_define =
                    Symbols::SymbolFactory::createConstant(variableName_, value, target_scope_name);
                sc->addConstant(symbol_to_define, target_scope_name); // Use current scope
            } else {
                symbol_to_define = Symbols::SymbolFactory::createVariable(variableName_, value,
                                                                          target_scope_name, variableType_);
                sc->addVariable(symbol_to_define, target_scope_name); // Use current scope
            }

        } catch (const Exception &) {
            throw;
        } catch (const std::exception & e) {
            throw Exception(e.what(), filename_, line_, column_);
        }
    }

    std::string toString() const override {
        return std::string("variable name: " + variableName_ +
                           " type: " + Symbols::Variables::TypeToString(variableType_));
    }

    const std::string & getNamespace() const { return ns; }
};

}  // namespace Interpreter

#endif  // INTERPRETER_DEFINE_VARIABLE_STATEMENT_NODE_HPP
