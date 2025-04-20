// ReturnStatementNode.hpp
#ifndef INTERPRETER_RETURN_STATEMENT_NODE_HPP
#define INTERPRETER_RETURN_STATEMENT_NODE_HPP

#include <memory>
#include <string>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/ReturnException.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

/**
 * @brief Statement node representing a return statement inside a function.
 */
class ReturnStatementNode : public StatementNode {
    std::unique_ptr<ExpressionNode> expr_;
  public:
    explicit ReturnStatementNode(std::unique_ptr<ExpressionNode> expr, const std::string & file_name, int line,
                                 size_t column) :
        StatementNode(file_name, line, column),
        expr_(std::move(expr)) {}

    void interpret(Interpreter & interpreter) const override {
        Symbols::Value retVal;
        if (expr_) {
            retVal = expr_->evaluate(interpreter);
        }
        throw ReturnException(retVal);
    }

    std::string toString() const override {
        return std::string("return") + (expr_ ? (" " + expr_->toString()) : std::string());
    }
};

}  // namespace Interpreter
#endif  // INTERPRETER_RETURN_STATEMENT_NODE_HPP
