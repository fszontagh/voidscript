 #ifndef INTERPRETER_FOR_STATEMENT_NODE_HPP
 #define INTERPRETER_FOR_STATEMENT_NODE_HPP

 #include <vector>
 #include <memory>
 #include <string>
 #include <stdexcept>
#include "Interpreter/StatementNode.hpp"
// Include for unified runtime Exception
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/ExpressionNode.hpp"
 #include "Symbols/Value.hpp"
 #include "Symbols/SymbolContainer.hpp"
 #include "Symbols/SymbolFactory.hpp"

 namespace Interpreter {

 /**
  * @brief Statement node representing a for-in loop over object members.
  */
 class ForStatementNode : public StatementNode {
   private:
    Symbols::Variables::Type keyType_;
    std::string keyName_;
    std::string valueName_;
    std::unique_ptr<ExpressionNode> iterableExpr_;
    std::vector<std::unique_ptr<StatementNode>> body_;

   public:
    ForStatementNode(Symbols::Variables::Type keyType,
                     std::string keyName,
                     std::string valueName,
                     std::unique_ptr<ExpressionNode> iterableExpr,
                     std::vector<std::unique_ptr<StatementNode>> body,
                     const std::string & file_name,
                     int line,
                     size_t column)
      : StatementNode(file_name, line, column),
        keyType_(keyType),
        keyName_(std::move(keyName)),
        valueName_(std::move(valueName)),
        iterableExpr_(std::move(iterableExpr)),
        body_(std::move(body)) {}

    void interpret(Interpreter & interpreter) const override {
        try {
            using namespace Symbols;
            auto iterableVal = iterableExpr_->evaluate(interpreter);
            if (iterableVal.getType() != Variables::Type::OBJECT) {
                throw Exception("For-in loop applied to non-object", filename_, line_, column_);
            }
            const auto & objMap = std::get<Value::ObjectMap>(iterableVal.get());
            auto * symContainer = SymbolContainer::instance();
            const std::string base_ns = symContainer->currentScopeName();
            const std::string var_ns  = base_ns + "::variables";
            for (const auto & entry : objMap) {
                const std::string & key = entry.first;
                Value keyVal(key);
                if (!symContainer->exists(keyName_, var_ns)) {
                    auto sym = SymbolFactory::createVariable(keyName_, keyVal, base_ns);
                    symContainer->add(sym);
                } else {
                    auto sym = symContainer->get(var_ns, keyName_);
                    sym->setValue(keyVal);
                }
                Value valVal = entry.second;
                if (!symContainer->exists(valueName_, var_ns)) {
                    auto sym = SymbolFactory::createVariable(valueName_, valVal, base_ns);
                    symContainer->add(sym);
                } else {
                    auto sym = symContainer->get(var_ns, valueName_);
                    sym->setValue(valVal);
                }
                for (const auto & stmt : body_) {
                    stmt->interpret(interpreter);
                }
            }
        } catch (const Exception &) {
            throw;
        } catch (const std::exception &e) {
            throw Exception(e.what(), filename_, line_, column_);
        }
    }

    std::string toString() const override {
        return "ForStatementNode at " + filename_ + ":" + std::to_string(line_);
    }
 };

} // namespace Interpreter

#endif // INTERPRETER_FOR_STATEMENT_NODE_HPP