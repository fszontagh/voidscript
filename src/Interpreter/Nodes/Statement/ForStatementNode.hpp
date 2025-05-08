#ifndef INTERPRETER_FOR_STATEMENT_NODE_HPP
#define INTERPRETER_FOR_STATEMENT_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/StatementNode.hpp"
// Include for unified runtime Exception
#include "Interpreter/ExpressionNode.hpp"
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
    Symbols::Variables::Type                    keyType_;
    std::string                                 keyName_;
    std::string                                 valueName_;
    std::unique_ptr<ExpressionNode>             iterableExpr_;
    std::vector<std::unique_ptr<StatementNode>> body_;
    std::string                                 loopScopeName_;

  public:
    ForStatementNode(Symbols::Variables::Type keyType, std::string keyName, std::string valueName,
                     std::unique_ptr<ExpressionNode> iterableExpr, std::vector<std::unique_ptr<StatementNode>> body,
                     std::string loopScopeName,
                     const std::string & file_name, int line, size_t column) :
        StatementNode(file_name, line, column),
        keyType_(keyType),
        keyName_(std::move(keyName)),
        valueName_(std::move(valueName)),
        iterableExpr_(std::move(iterableExpr)),
        body_(std::move(body)),
        loopScopeName_(std::move(loopScopeName)) {
        // Ensure the loop scope name contains "for_" for proper scope handling
        if (loopScopeName_.find("for_") == std::string::npos) {
            loopScopeName_ = loopScopeName_ + "::for_" + std::to_string(line) + "_" + std::to_string(column);
        }
    }

    void interpret(Interpreter & interpreter) const override {
        bool entered_scope = false;
        try {
            using namespace Symbols;
            auto iterableVal = iterableExpr_->evaluate(interpreter);
            if (iterableVal.getType() != Variables::Type::OBJECT) {
                throw Exception("For-in loop applied to non-object", filename_, line_, column_);
            }
            const auto &      objMap       = std::get<Value::ObjectMap>(iterableVal.get());
            auto *            symContainer = SymbolContainer::instance();
            
            // Create and enter the loop scope only once
            if (!symContainer->getScopeTable(loopScopeName_)) {
                symContainer->create(loopScopeName_);
            }
            symContainer->enter(loopScopeName_);
            entered_scope = true;
            
            // Create the key and value variables once before the loop
            auto keySym = SymbolFactory::createVariable(keyName_, Value(""), loopScopeName_);
            auto valSym = SymbolFactory::createVariable(valueName_, Value::makeNull(Variables::Type::NULL_TYPE), loopScopeName_);
            symContainer->add(keySym);
            symContainer->add(valSym);
            
            for (const auto & entry : objMap) {
                const std::string & key = entry.first;
                Value               keyVal(key);
                keySym->setValue(keyVal);
                
                Value valVal = entry.second;
                valSym->setValue(valVal);
                
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

    std::string toString() const override { return "ForStatementNode at " + filename_ + ":" + std::to_string(line_); }
};

}  // namespace Interpreter

#endif  // INTERPRETER_FOR_STATEMENT_NODE_HPP
