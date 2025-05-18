#ifndef INTERPRETER_WHILE_STATEMENT_NODE_HPP
#define INTERPRETER_WHILE_STATEMENT_NODE_HPP

#include <memory>
#include <string>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Symbols/SymbolContainer.hpp"

namespace Interpreter {

class WhileStatementNode : public StatementNode {
  private:
    std::unique_ptr<ExpressionNode>             conditionExpr_;
    std::vector<std::unique_ptr<StatementNode>> body_;
    std::string                                 loopScopeName_;

  public:
    WhileStatementNode(std::unique_ptr<ExpressionNode> conditionExpr, std::vector<std::unique_ptr<StatementNode>> body,
                       const std::string & file_name, int line, size_t column) :
        StatementNode(file_name, line, column),
        conditionExpr_(std::move(conditionExpr)),
        body_(std::move(body)) {
        // Create a unique scope name for this while loop
        loopScopeName_ = Symbols::SymbolContainer::instance()->currentScopeName() + "::while_" +
                        std::to_string(line) + "_" + std::to_string(column);
    }

    void interpret(Interpreter & interpreter) const override {
        bool entered_scope = false;
        try {
            auto* sc = Symbols::SymbolContainer::instance();

            // Create and enter the loop scope only once
            if (!sc->getScopeTable(loopScopeName_)) {
                sc->create(loopScopeName_);
            }
            sc->enter(loopScopeName_);
            entered_scope = true;

            bool cond;
            while (true) {
                auto val = conditionExpr_->evaluate(interpreter);
                if (val->getType() != Symbols::Variables::Type::BOOLEAN) {
                    throw Exception("Condition did not evaluate to boolean: " + conditionExpr_->toString(), filename_,
                                    line_, column_);
                }
                cond = val->get<bool>();
                if (!cond) {
                    break;
                }

                for (const auto & stmt : body_) {
                    stmt->interpret(interpreter);
                }
            }
        } catch (const Exception &) {
            if (entered_scope) {
                Symbols::SymbolContainer::instance()->enterPreviousScope();
            }
            throw;
        } catch (const std::exception & e) {
            if (entered_scope) {
                Symbols::SymbolContainer::instance()->enterPreviousScope();
            }
            throw Exception(e.what(), filename_, line_, column_);
        }

        // Exit the loop scope
        if (entered_scope) {
            Symbols::SymbolContainer::instance()->enterPreviousScope();
        }
    }

    std::string toString() const override { return "WhileStatementNode at " + filename_ + ":" + std::to_string(line_); }
};

}  // namespace Interpreter

#endif  // INTERPRETER_WHILE_STATEMENT_NODE_HPP
