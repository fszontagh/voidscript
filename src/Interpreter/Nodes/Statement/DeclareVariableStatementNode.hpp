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
            Symbols::Value value = expression_->evaluate(interpreter);
            const std::string var_ns = ns + "::variables";
            if (Symbols::SymbolContainer::instance()->exists(variableName_, var_ns)) {
                throw Exception("Variable already declared: " + variableName_, filename_, line_, column_);
            }
            if (value.getType() == Symbols::Variables::Type::NULL_TYPE) {
                value = Symbols::Value::makeNull(variableType_);
            }

            if (value.getType() != variableType_) {
                using namespace Symbols::Variables;
                std::string expected = TypeToString(variableType_);
                std::string actual   = TypeToString(value.getType());
                throw Exception("Type mismatch for variable '" + variableName_ + "': expected '" + expected +
                                    "' but got '" + actual + "'",
                                filename_, line_, column_);
            }

            // Create a constant or variable symbol
            std::shared_ptr<Symbols::Symbol> variable;
            if (isConst_) {
                variable = Symbols::SymbolFactory::createConstant(variableName_, value, ns);
            } else {
                variable = Symbols::SymbolFactory::createVariable(variableName_, value, ns, variableType_);
            }
            Symbols::SymbolContainer::instance()->add(variable);
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

    const std::string& getNamespace() const { return ns; }
};

}  // namespace Interpreter

#endif  // INTERPRETER_DEFINE_VARIABLE_STATEMENT_NODE_HPP
