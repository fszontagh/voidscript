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
        loopScopeName_(std::move(loopScopeName)) {}

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
            
            if (!loopScopeName_.empty()) {
                symContainer->enter(loopScopeName_);
                entered_scope = true;
            } else {
                throw Exception("Internal error: ForStatementNode missing loop scope name.", filename_, line_, column_);
            }
            
            for (const auto & entry : objMap) {
                const std::string & key = entry.first;
                Value               keyVal(key);
                if (!symContainer->findSymbol(keyName_)) {
                    auto sym = SymbolFactory::createVariable(keyName_, keyVal, loopScopeName_);
                    symContainer->add(sym);
                } else {
                    auto sym = symContainer->findSymbol(keyName_);
                    if(sym) sym->setValue(keyVal);
                }
                
                Value valVal = entry.second;
                if (!symContainer->findSymbol(valueName_)) {
                    auto sym = SymbolFactory::createVariable(valueName_, valVal, loopScopeName_);
                    symContainer->add(sym);
                } else {
                    auto sym = symContainer->findSymbol(valueName_);
                    if(sym) sym->setValue(valVal);
                }
                
                for (const auto & stmt : body_) {
                    stmt->interpret(interpreter);
                }
            }
            
            if (entered_scope) {
                symContainer->enterPreviousScope();
            }
        } catch (const Exception &e) {
            if (entered_scope) Symbols::SymbolContainer::instance()->enterPreviousScope();
            throw;
        } catch (const std::exception & e) {
            if (entered_scope) Symbols::SymbolContainer::instance()->enterPreviousScope();
            throw Exception(e.what(), filename_, line_, column_);
        }
    }

    std::string toString() const override { return "ForStatementNode at " + filename_ + ":" + std::to_string(line_); }
};

}  // namespace Interpreter

#endif  // INTERPRETER_FOR_STATEMENT_NODE_HPP
