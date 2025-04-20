#ifndef INTERPRETER_MEMBER_EXPRESSION_NODE_HPP
#define INTERPRETER_MEMBER_EXPRESSION_NODE_HPP

#include <memory>
#include <stdexcept>

#include "Interpreter/ExpressionNode.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

// Expression node for member access: object->property
class MemberExpressionNode : public ExpressionNode {
  public:
    MemberExpressionNode(std::unique_ptr<ExpressionNode> objectExpr, std::string propertyName) :
        objectExpr_(std::move(objectExpr)),
        propertyName_(std::move(propertyName)) {}

    Symbols::Value evaluate(Interpreter & interpreter) const override {
        Symbols::Value objVal = objectExpr_->evaluate(interpreter);
        if (objVal.getType() != Symbols::Variables::Type::OBJECT) {
            throw std::runtime_error("Attempted to access member '" + propertyName_ + "' of non-object");
        }
        const auto & map = std::get<Symbols::Value::ObjectMap>(objVal.get());
        auto         it  = map.find(propertyName_);
        if (it == map.end()) {
            throw std::runtime_error("Property '" + propertyName_ + "' not found on object");
        }
        return it->second;
    }

    std::string toString() const override { return objectExpr_->toString() + "->" + propertyName_; }

  private:
    std::unique_ptr<ExpressionNode> objectExpr_;
    std::string                     propertyName_;
};

}  // namespace Interpreter

#endif  // INTERPRETER_MEMBER_EXPRESSION_NODE_HPP
