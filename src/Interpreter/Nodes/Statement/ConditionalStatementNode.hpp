#ifndef INTERPRETER_CONDITIONAL_STATEMENT_NODE_HPP
#define INTERPRETER_CONDITIONAL_STATEMENT_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/StatementNode.hpp"
// Include for unified runtime Exception
#include "Interpreter/BreakException.hpp"
#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"

namespace Interpreter {

/**
 * @brief Statement node representing an if-else conditional block.
 */
class ConditionalStatementNode : public StatementNode {
    std::unique_ptr<ExpressionNode>             condition_;
    std::vector<std::unique_ptr<StatementNode>> thenBranch_;
    std::vector<std::unique_ptr<StatementNode>> elseBranch_;

  public:
    ConditionalStatementNode(std::unique_ptr<ExpressionNode>             condition,
                             std::vector<std::unique_ptr<StatementNode>> thenBranch,
                             std::vector<std::unique_ptr<StatementNode>> elseBranch, const std::string & file_name,
                             int line, size_t column) :
        StatementNode(file_name, line, column),
        condition_(std::move(condition)),
        thenBranch_(std::move(thenBranch)),
        elseBranch_(std::move(elseBranch)) {}

    void interpret(class Interpreter & interpreter) const override {
        try {
            auto val  = condition_->evaluate(interpreter, filename_, line_, column_);
            bool cond = false;
            if (val == Symbols::Variables::Type::BOOLEAN) {
                cond = val;
            } else {
                throw Exception("Condition did not evaluate to boolean: " + condition_->toString(), filename_, line_,
                                column_);
            }
            const auto & branch = cond ? thenBranch_ : elseBranch_;
            // Only iterate if the branch is not empty
            if (!branch.empty()) {
                for (const auto & stmt : branch) {
                    stmt->interpret(interpreter);
                }
            }
        } catch (const BreakException &) {
            // Re-throw BreakException so loop nodes can catch it
            throw;
        } catch (const Exception &) {
            throw;
        } catch (const std::exception & e) {
            throw Exception(e.what(), filename_, line_, column_);
        }
    }

    std::string toString() const override {
        return "ConditionalStatementNode at " + filename_ + ":" + std::to_string(line_);
    }
};

}  // namespace Interpreter

#endif  // INTERPRETER_CONDITIONAL_STATEMENT_NODE_HPP
