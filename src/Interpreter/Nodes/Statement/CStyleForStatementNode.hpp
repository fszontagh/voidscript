#ifndef INTERPRETER_CSTYLE_FOR_STATEMENT_NODE_HPP
#define INTERPRETER_CSTYLE_FOR_STATEMENT_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/BreakException.hpp"
#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp" // Required for Symbols::ValuePtr(true)

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
    // loopScopeName_ is no longer strictly necessary for init variable scoping,
    // but might be useful for debugging or if loop body needs a predictable name base.
    // For now, its direct usage for init var scoping is removed.
    // std::string                                 loopScopeName_; 

  public:
    CStyleForStatementNode(std::unique_ptr<StatementNode> initStmt, std::unique_ptr<ExpressionNode> condExpr,
                           std::unique_ptr<StatementNode> incrStmt, std::vector<std::unique_ptr<StatementNode>> body,
                           const std::string & file_name, int line, size_t column) :
        StatementNode(file_name, line, column),
        initStmt_(std::move(initStmt)),
        condExpr_(std::move(condExpr)),
        incrStmt_(std::move(incrStmt)),
        body_(std::move(body)) {
        // loopScopeName_ initialization removed as its primary purpose changed.
        // If needed for other reasons, it would be:
        // loopScopeName_ = Symbols::SymbolContainer::instance()->currentScopeName() + "::for_" + std::to_string(line) +
        //                  "_" + std::to_string(column);
    }

    void interpret(Interpreter & interpreter) const override {
        // Get symbol container instance
        auto * symContainer = Symbols::SymbolContainer::instance();

        // 1. Execute initialization statement in the current (parent) scope
        if (initStmt_) {
            initStmt_->interpret(interpreter);
        }

        // Flag to track if the specific loop scope was entered
        bool entered_loop_scope = false;
        // Define the name for the loop's own operational scope (for body, condition, increment)
        // This scope is nested within the parent scope where initStmt_ vars now reside.
        std::string runtime_loop_scope_name = symContainer->currentScopeName() + 
                                           Symbols::SymbolContainer::SCOPE_SEPARATOR + "for_" +
                                           std::to_string(line_) + "_" + 
                                           std::to_string(column_);
        try {
            // 2. Create and enter the specific operational scope for the loop
            if (!symContainer->getScopeTable(runtime_loop_scope_name)) {
                symContainer->create(runtime_loop_scope_name);
            } else {
                // If scope exists (e.g. from a previous identical loop construct not recommended),
                // for now, just enter. Future: consider if SymbolTable needs a clearVariables()
                symContainer->enter(runtime_loop_scope_name);
            }
            entered_loop_scope = true;

            // 3. Loop condition, body, and increment execute within runtime_loop_scope_name
            while (true) {
                // Evaluate condition (in loop scope, can access parent scope vars like $i)
                Symbols::ValuePtr condVal;
                if (condExpr_) { // Condition can be null for infinite loops like for(;;)
                    condVal = condExpr_->evaluate(interpreter);
                } else {
                    // If no condition expression, it's an infinite loop, effectively true.
                    condVal = Symbols::ValuePtr(true); 
                }
                
                if (condVal != Symbols::Variables::Type::BOOLEAN) {
                    // No need to manually pop scope here, catch block will handle it
                    throw Exception("For loop condition not boolean", filename_, line_, column_);
                }
                bool shouldContinue = condVal; // Implicit conversion from ValuePtr to bool
                if (!shouldContinue) {
                    break;
                }

                // Execute body (in loop scope)
                try {
                    for (const auto & stmt : body_) {
                        if (stmt) {
                            stmt->interpret(interpreter);
                        }
                    }
                } catch (const BreakException &) {
                    // Break out of the C-style for loop
                    break;
                }

                // Execute increment (in loop scope)
                if (incrStmt_) {
                    incrStmt_->interpret(interpreter);
                }
            }
        } catch (const Exception &) { // Catch voidscript specific exceptions
            if (entered_loop_scope) {
                symContainer->enterPreviousScope();  // Ensure exit on exception
            }
            throw; // Re-throw
        } catch (const std::exception & e) { // Catch standard C++ exceptions
            if (entered_loop_scope) {
                symContainer->enterPreviousScope();  // Ensure exit on exception
            }
            throw Exception(e.what(), filename_, line_, column_); // Wrap and re-throw
        }

        // 4. Exit the loop's specific operational scope
        if (entered_loop_scope) {
            symContainer->enterPreviousScope();
        }
    }

    std::string toString() const override {
        return "CStyleForStatementNode at " + filename_ + ":" + std::to_string(line_);
    }
};

}  // namespace Interpreter

#endif  // INTERPRETER_CSTYLE_FOR_STATEMENT_NODE_HPP
