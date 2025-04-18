#ifndef INTERPRETER_DEFINE_VARIABLE_STATEMENT_NODE_HPP
#define INTERPRETER_DEFINE_VARIABLE_STATEMENT_NODE_HPP

#include <memory>
#include <string>
#include <utility>

#include "ExpressionNode.hpp"
#include "Interpreter.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"

namespace Interpreter {

class DeclareVariableStatementNode : public StatementNode {
    std::string                     variableName_;
    Symbols::Variables::Type        variableType_;
    std::unique_ptr<ExpressionNode> expression_;
    std::string                     ns;


  public:
    DeclareVariableStatementNode(std::string name, const std::string & ns, Symbols::Variables::Type type,
                                 std::unique_ptr<ExpressionNode> expr, const std::string & file_name, int file_line,
                                 size_t line_column) :
        StatementNode(file_name, file_line, line_column),
        variableName_(std::move(name)),
        variableType_(type),
        expression_(std::move(expr)),
        ns(ns) {}

    void interpret(Interpreter & interpreter) const override {
        // Evaluate the expression and enforce declared type matches actual value type
        Symbols::Value value = expression_->evaluate(interpreter);
        // Check for duplicate declaration
        if (Symbols::SymbolContainer::instance()->exists(variableName_)) {
            throw std::runtime_error("Variable already declared: " + variableName_ + " File: " + filename_ +
                                     ", Line: " + std::to_string(line_) + ", Column: " + std::to_string(column_));
        }
        // Enforce type correctness: the evaluated value must match the declared type
        if (value.getType() != variableType_) {
            using namespace Symbols::Variables;
            std::string expected = TypeToString(variableType_);
            std::string actual   = TypeToString(value.getType());
            throw std::runtime_error("Type mismatch for variable '" + variableName_ +
                                     "': expected '" + expected + "' but got '" + actual +
                                     "' File: " + filename_ + ", Line: " + std::to_string(line_) +
                                     ", Column: " + std::to_string(column_));
        }
        // Create and add the variable symbol
        const auto variable = Symbols::SymbolFactory::createVariable(variableName_, value, ns, variableType_);
        Symbols::SymbolContainer::instance()->add(variable);
    }

    std::string toString() const override {
        return std::string("variable name: " + variableName_ +
                           " type: " + Symbols::Variables::TypeToString(variableType_));
    }
};

}  // namespace Interpreter

#endif  // INTERPRETER_DEFINE_VARIABLE_STATEMENT_NODE_HPP
