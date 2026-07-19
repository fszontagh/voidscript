#ifndef INTERPRETER_TERNARY_EXPRESSION_NODE_HPP
#define INTERPRETER_TERNARY_EXPRESSION_NODE_HPP

#include <memory>
#include <string>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

/**
 * @brief The ternary conditional: cond ? thenBranch : elseBranch.
 *
 * Only the selected branch is evaluated, so side effects in the untaken branch do
 * not happen and an expensive or failing branch is never run.
 */
class TernaryExpressionNode : public ExpressionNode {
  private:
    std::unique_ptr<ExpressionNode> condition_;
    std::unique_ptr<ExpressionNode> thenBranch_;
    std::unique_ptr<ExpressionNode> elseBranch_;

  public:
    TernaryExpressionNode(std::unique_ptr<ExpressionNode> condition, std::unique_ptr<ExpressionNode> thenBranch,
                          std::unique_ptr<ExpressionNode> elseBranch, const std::string & file, int line,
                          size_t column) :
        condition_(std::move(condition)),
        thenBranch_(std::move(thenBranch)),
        elseBranch_(std::move(elseBranch)) {
        this->filename = file;
        this->line     = line;
        this->column   = column;
    }

    Symbols::ValuePtr evaluate(Interpreter & interpreter, std::string /*filename*/, int /*line*/,
                               size_t /*column*/) const override {
        Symbols::ValuePtr condValue = condition_->evaluate(interpreter, filename, line, column);

        if (condValue->is_null()) {
            throw Exception("Ternary condition must not be null", filename, line, column);
        }
        if (condValue.getType() != Symbols::Variables::Type::BOOLEAN) {
            throw Exception("Ternary condition must be a boolean, got '" +
                                Symbols::Variables::TypeToString(condValue.getType()) + "'",
                            filename, line, column);
        }

        // Evaluate only the branch actually taken.
        return condValue->get<bool>() ? thenBranch_->evaluate(interpreter, filename, line, column)
                                      : elseBranch_->evaluate(interpreter, filename, line, column);
    }

    std::string toString() const override {
        return "(" + condition_->toString() + " ? " + thenBranch_->toString() + " : " + elseBranch_->toString() + ")";
    }
};

}  // namespace Interpreter

#endif  // INTERPRETER_TERNARY_EXPRESSION_NODE_HPP
