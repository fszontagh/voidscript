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
            auto value = expression_->evaluate(interpreter);

            auto * sc = Symbols::SymbolContainer::instance();

            // Use the CURRENT scope from SymbolContainer for definitions and checks
            std::string current_runtime_scope_name = sc->currentScopeName();
            auto        targetTable                = sc->getScopeTable(current_runtime_scope_name);

            if (!targetTable) {
                // This should ideally not happen if parser/caller creates scopes correctly
                throw Exception("Current runtime scope '" + current_runtime_scope_name +
                                    "' for variable declaration does not exist",
                                filename_, line_, column_);
            }

            // Check if variable/constant already exists in the target scope's table
            auto existing_var = targetTable->get(Symbols::SymbolContainer::DEFAULT_VARIABLES_SCOPE, variableName_);
            if (existing_var) {
                // If we're in a loop scope (scope name contains "for_" or "while_"), allow redeclaration
                if (current_runtime_scope_name.find("for_") != std::string::npos ||
                    current_runtime_scope_name.find("while_") != std::string::npos) {
                    // Update the existing variable's value
                    existing_var->setValue(value);
                    return;
                }
                throw Exception(
                    "Variable '" + variableName_ + "' already declared in scope '" + current_runtime_scope_name + "'",
                    filename_, line_, column_);
            }

            Symbols::SymbolPtr existing_const_check =
                targetTable->get(Symbols::SymbolContainer::DEFAULT_CONSTANTS_SCOPE, variableName_);
            if (existing_const_check) {
                throw Exception(
                    "Cannot redefine constant '" + variableName_ + "' in scope '" + current_runtime_scope_name + "'",
                    filename_, line_, column_);
            }

            if (value == Symbols::Variables::Type::NULL_TYPE) {
                value.setType(variableType_);
            }

            if (value != variableType_) {
                std::string expected = Symbols::Variables::TypeToString(variableType_);
                std::string actual   = Symbols::Variables::TypeToString(value);
                throw Exception("Type mismatch for variable '" + variableName_ + "': expected '" + expected +
                                    "' but got '" + actual + "' in scope '" + current_runtime_scope_name + "'",
                                filename_, line_, column_);
            }

            // Create a constant or variable symbol
            // The symbol's own context should be this current_runtime_scope_name
            std::shared_ptr<Symbols::Symbol> symbol_to_define;
            if (isConst_) {
                symbol_to_define =
                    Symbols::SymbolFactory::createConstant(variableName_, value.clone(), current_runtime_scope_name);
            } else {
                symbol_to_define = Symbols::SymbolFactory::createVariable(variableName_, value.clone(),
                                                                          current_runtime_scope_name, variableType_);
            }
            // sc->add() uses SymbolContainer::currentScopeName() internally, which is current_runtime_scope_name
            sc->add(symbol_to_define);
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
