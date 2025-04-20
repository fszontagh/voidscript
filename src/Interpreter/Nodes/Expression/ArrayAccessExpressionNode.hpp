#ifndef INTERPRETER_ARRAY_ACCESS_EXPRESSION_NODE_HPP
#define INTERPRETER_ARRAY_ACCESS_EXPRESSION_NODE_HPP

#include <memory>
#include <string>
#include "Interpreter/Interpreter.hpp"

#include "Interpreter/ExpressionNode.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

/**
  * @brief Expression node for dynamic array/object indexing: expr[index]
  */
/**
 * @brief Expression node for dynamic array/object indexing: expr[index]
 */
class ArrayAccessExpressionNode : public ExpressionNode {
  public:
    ArrayAccessExpressionNode(std::unique_ptr<ExpressionNode> arrayExpr,
                              std::unique_ptr<ExpressionNode> indexExpr,
                              const std::string & filename,
                              int line,
                              size_t column) :
        arrayExpr_(std::move(arrayExpr)),
        indexExpr_(std::move(indexExpr)),
        filename_(filename),
        line_(line),
        column_(column) {}

    Symbols::Value evaluate(Interpreter & interpreter) const override {
        // Evaluate the container (object or array)
        Symbols::Value container = arrayExpr_->evaluate(interpreter);
        if (container.getType() != Symbols::Variables::Type::OBJECT) {
            throw Exception("Attempted to index non-array", filename_, line_, column_);
        }
        const auto & map = std::get<Symbols::Value::ObjectMap>(container.get());
        // Evaluate the index
        Symbols::Value idxVal = indexExpr_->evaluate(interpreter);
        std::string    key;
        if (idxVal.getType() == Symbols::Variables::Type::INTEGER) {
            key = std::to_string(idxVal.get<int>());
        } else if (idxVal.getType() == Symbols::Variables::Type::STRING) {
            key = idxVal.get<std::string>();
        } else {
            throw Exception("Array index must be integer or string", filename_, line_, column_);
        }
        auto it = map.find(key);
        if (it == map.end()) {
            throw Exception("Index not found: " + key, filename_, line_, column_);
        }
        return it->second;
    }

    std::string toString() const override { return arrayExpr_->toString() + "[" + indexExpr_->toString() + "]"; }

  private:
    std::unique_ptr<ExpressionNode> arrayExpr_;
    std::unique_ptr<ExpressionNode> indexExpr_;
    std::string                     filename_;
    int                             line_;
    size_t                          column_;
};

}  // namespace Interpreter

#endif  // INTERPRETER_ARRAY_ACCESS_EXPRESSION_NODE_HPP
