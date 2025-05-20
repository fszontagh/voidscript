#ifndef INTERPRETER_CSTYLE_FOR_STATEMENT_NODE_HPP
#define INTERPRETER_CSTYLE_FOR_STATEMENT_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Symbols/SymbolContainer.hpp"

namespace Interpreter {

/**
 * @brief Statement node representing a C-style for loop: for(init; cond; incr) { body }
 */
class CStyleForStatementNode : public StatementNode {
  private:
    std::unique_ptr<StatementNode>              initStmt_;
    std::unique_ptr<ExpressionNode>             condExpr_;
    std::unique_ptr<StatementNode>              incrStmt_;
    std::vector<std::unique_ptr<StatementNode>> body_;
    std::string                                 loopScopeName_;

  public:
    CStyleForStatementNode(std::unique_ptr<StatementNode> initStmt, std::unique_ptr<ExpressionNode> condExpr,
                           std::unique_ptr<StatementNode> incrStmt, std::vector<std::unique_ptr<StatementNode>> body,
                           const std::string & file_name, int line, size_t column) :
        StatementNode(file_name, line, column),
        initStmt_(std::move(initStmt)),
        condExpr_(std::move(condExpr)),
        incrStmt_(std::move(incrStmt)),
        body_(std::move(body)) {
        // Create a unique scope name for this for loop
        loopScopeName_ = Symbols::SymbolContainer::instance()->currentScopeName() + "::for_" + std::to_string(line) +
                         "_" + std::to_string(column);
    }

    void interpret(Interpreter & interpreter) const override {
        bool entered_scope = false;
        try {
            using namespace Symbols;
            auto * symContainer = SymbolContainer::instance();

            // Create and enter the loop scope only once
            if (!symContainer->getScopeTable(loopScopeName_)) {
                symContainer->create(loopScopeName_);
            }
            symContainer->enter(loopScopeName_);
            entered_scope = true;

            initStmt_->interpret(interpreter);

            // Loop condition and body (executed within loop scope)
            while (true) {
                auto condVal = condExpr_->evaluate(interpreter);
                if (condVal != Variables::Type::BOOLEAN) {
                    if (entered_scope) {
                        symContainer->enterPreviousScope();  // Exit scope before throwing
                    }
                    throw Exception("For loop condition not boolean", filename_, line_, column_);
                }
                bool shouldContinue = condVal;
                if (!shouldContinue) {
                    break;
                }

                for (const auto & stmt : body_) {
                    if (stmt) {
                        stmt->interpret(interpreter);
                    }
                }

                if (incrStmt_) {
                    incrStmt_->interpret(interpreter);
                }
            }
        } catch (const Exception & e) {
            if (entered_scope) {
                Symbols::SymbolContainer::instance()->enterPreviousScope();  // Ensure exit on exception
            }
            throw;
        } catch (const std::exception & e) {
            if (entered_scope) {
                Symbols::SymbolContainer::instance()->enterPreviousScope();  // Ensure exit on exception
            }
            throw Exception(e.what(), filename_, line_, column_);
        }

        // Exit the loop scope only once at the end
        if (entered_scope) {
            Symbols::SymbolContainer::instance()->enterPreviousScope();
        }
    }

    std::string toString() const override {
        return "CStyleForStatementNode at " + filename_ + ":" + std::to_string(line_);
    }
};

}  // namespace Interpreter

#endif  // INTERPRETER_CSTYLE_FOR_STATEMENT_NODE_HPP
