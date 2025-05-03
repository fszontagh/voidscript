#ifndef INTERPRETER_WHILE_STATEMENT_NODE_HPP
#define INTERPRETER_WHILE_STATEMENT_NODE_HPP

#include <memory>
#include <string>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/StatementNode.hpp"

namespace Interpreter {

class WhileStatementNode : public StatementNode {
  private:
    std::unique_ptr<ExpressionNode>             conditionExpr_;
    std::vector<std::unique_ptr<StatementNode>> body_;

  public:
    WhileStatementNode(std::unique_ptr<ExpressionNode> conditionExpr, std::vector<std::unique_ptr<StatementNode>> body,
                       const std::string & file_name, int line, size_t column) :
        StatementNode(file_name, line, column),
        conditionExpr_(std::move(conditionExpr)),
        body_(std::move(body)) {}

    void interpret(Interpreter & interpreter) const override {
        try {
            bool cond;
            while (true) {
                auto val = conditionExpr_->evaluate(interpreter);
                if (val.getType() != Symbols::Variables::Type::BOOLEAN) {
                    throw Exception("Condition did not evaluate to boolean: " + conditionExpr_->toString(), filename_,
                                    line_, column_);
                }
                cond = val.get<bool>();
                if (!cond) {
                    break;
                }

                for (const auto & stmt : body_) {
                    stmt->interpret(interpreter);
                }
            }
        } catch (const Exception &) {
            throw;
        } catch (const std::exception & e) {
            throw Exception(e.what(), filename_, line_, column_);
        }
    }

    std::string toString() const override { return "WhileStatementNode at " + filename_ + ":" + std::to_string(line_); }
};

}  // namespace Interpreter

#endif  // INTERPRETER_WHILE_STATEMENT_NODE_HPP
