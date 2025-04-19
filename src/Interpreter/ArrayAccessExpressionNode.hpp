 #ifndef INTERPRETER_ARRAY_ACCESS_EXPRESSION_NODE_HPP
 #define INTERPRETER_ARRAY_ACCESS_EXPRESSION_NODE_HPP

 #include "ExpressionNode.hpp"
 #include "Symbols/Value.hpp"
 #include <stdexcept>
 #include <string>

 namespace Interpreter {

 /**
  * @brief Expression node for dynamic array/object indexing: expr[index]
  */
 class ArrayAccessExpressionNode : public ExpressionNode {
   private:
    std::unique_ptr<ExpressionNode> arrayExpr_;
    std::unique_ptr<ExpressionNode> indexExpr_;

   public:
    ArrayAccessExpressionNode(std::unique_ptr<ExpressionNode> arrayExpr,
                              std::unique_ptr<ExpressionNode> indexExpr)
        : arrayExpr_(std::move(arrayExpr)), indexExpr_(std::move(indexExpr)) {}

    Symbols::Value evaluate(Interpreter &interpreter) const override {
        // Evaluate the container (object or array)
        Symbols::Value container = arrayExpr_->evaluate(interpreter);
        if (container.getType() != Symbols::Variables::Type::OBJECT) {
            throw std::runtime_error("Attempted to index non-array");
        }
        const auto & map = std::get<Symbols::Value::ObjectMap>(container.get());
        // Evaluate the index
        Symbols::Value idxVal = indexExpr_->evaluate(interpreter);
        std::string key;
        if (idxVal.getType() == Symbols::Variables::Type::INTEGER) {
            key = std::to_string(idxVal.get<int>());
        } else if (idxVal.getType() == Symbols::Variables::Type::STRING) {
            key = idxVal.get<std::string>();
        } else {
            throw std::runtime_error("Array index must be integer or string");
        }
        auto it = map.find(key);
        if (it == map.end()) {
            throw std::runtime_error("Index not found: " + key);
        }
        return it->second;
    }

    std::string toString() const override {
        return arrayExpr_->toString() + "[" + indexExpr_->toString() + "]";
    }
};

} // namespace Interpreter

 #endif // INTERPRETER_ARRAY_ACCESS_EXPRESSION_NODE_HPP