#ifndef INTERPRETER_FOR_STATEMENT_NODE_HPP
#define INTERPRETER_FOR_STATEMENT_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/StatementNode.hpp"
// Include for unified runtime Exception
#include "Interpreter/BreakException.hpp"
#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Nodes/Expression/IdentifierExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

/**
  * @brief Statement node representing a for-in loop over object members.
  */
class ForStatementNode : public StatementNode {
  private:
    // Symbols::Variables::Type                    keyType_;
    std::string                                 keyName_;
    std::string                                 valueName_;
    std::unique_ptr<ExpressionNode>             iterableExpr_;
    std::vector<std::unique_ptr<StatementNode>> body_;
    std::string                                 loopScopeName_;

  public:
    ForStatementNode(Symbols::Variables::Type keyType, std::string keyName, std::string valueName,
                     std::unique_ptr<ExpressionNode> iterableExpr, std::vector<std::unique_ptr<StatementNode>> body,
                     std::string loopScopeName, const std::string & file_name, int line, size_t column) :
        StatementNode(file_name, line, column),
        //  keyType_(keyType),
        keyName_(std::move(keyName)),
        valueName_(std::move(valueName)),
        iterableExpr_(std::move(iterableExpr)),
        body_(std::move(body)),
        loopScopeName_(std::move(loopScopeName)) {
        // Ensure the loop scope name contains "for_" for proper scope handling
        if (loopScopeName_.find("for_") == std::string::npos) {
            loopScopeName_ = loopScopeName_ + Symbols::SymbolContainer::SCOPE_SEPARATOR + "for_" +
                             std::to_string(line) + "_" + std::to_string(column);
        }
    }

    void interpret(Interpreter & interpreter) const override {
        bool entered_scope = false;
        try {
            auto *symContainer = Symbols::SymbolContainer::instance();
            
            // IMPORTANT: Evaluate the iterable expression FIRST, before creating any new scopes
            // This ensures that function parameters (like $class) can be found in the current scope
            Symbols::ValuePtr iterableVal = iterableExpr_->evaluate(interpreter);
            
            if (iterableVal != Symbols::Variables::Type::OBJECT) {
                throw Exception("For-in loop applied to non-object: " + Symbols::Variables::TypeToString(iterableVal),
                                filename_, line_, column_);
            }
            const Symbols::ObjectMap & objMap = iterableVal;

            // Build loop scope name based on current runtime scope, not parse-time scope
            std::string runtime_loop_scope = symContainer->currentScopeName() +
                                            Symbols::SymbolContainer::SCOPE_SEPARATOR + "for_" +
                                            std::to_string(line_) + "_" + std::to_string(column_);

            // Create and enter the loop scope
            // The scope creation automatically maintains the scope stack hierarchy,
            // which preserves access to parent scopes (including function call scopes with parameters)
            if (!symContainer->getScopeTable(runtime_loop_scope)) {
                symContainer->create(runtime_loop_scope);
                entered_scope = true;
            } else {
                symContainer->enter(runtime_loop_scope);
                entered_scope = true;
            }

            // Create the key and value variables once before the loop
            auto keySym = Symbols::SymbolFactory::createVariable(
                keyName_, Symbols::ValuePtr::null(Symbols::Variables::Type::STRING), runtime_loop_scope);
            auto valSym = Symbols::SymbolFactory::createVariable(
                valueName_, Symbols::ValuePtr::null(Symbols::Variables::Type::OBJECT), runtime_loop_scope);
            symContainer->add(keySym);
            symContainer->add(valSym);

            for (const auto & entry : objMap) {
                const std::string & key    = entry.first;
                auto                keyVal = Symbols::ValuePtr(key);
                keySym->setValue(keyVal);
                valSym->setValue(entry.second);

                try {
                    for (const auto & stmt : body_) {
                        stmt->interpret(interpreter);
                    }
                } catch (const BreakException &) {
                    // Break out of the for-in loop
                    break;
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

    std::string toString() const override { return "ForStatementNode at " + filename_ + ":" + std::to_string(line_); }
};

}  // namespace Interpreter

#endif  // INTERPRETER_FOR_STATEMENT_NODE_HPP
