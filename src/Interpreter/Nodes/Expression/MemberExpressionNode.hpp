#ifndef INTERPRETER_MEMBER_EXPRESSION_NODE_HPP
#define INTERPRETER_MEMBER_EXPRESSION_NODE_HPP

#include <memory>
#include <string>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

// Expression node for member access: object->property
class MemberExpressionNode : public ExpressionNode {
  public:
    MemberExpressionNode(std::unique_ptr<ExpressionNode> objectExpr, std::string propertyName,
                         const std::string & filename, int line, size_t column) :
        objectExpr_(std::move(objectExpr)),
        propertyName_(std::move(propertyName)),
        filename_(filename),
        line_(line),
        column_(column) {}

        Symbols::Value::ValuePtr evaluate(Interpreter & interpreter) const override {
        auto objVal = objectExpr_->evaluate(interpreter);

        // Allow member access on plain objects and class instances
        if (objVal->getType() != Symbols::Variables::Type::OBJECT &&
            objVal->getType() != Symbols::Variables::Type::CLASS) {
            throw Exception("Attempted to access member '" + propertyName_ + "' of non-object", filename_, line_,
                            column_);
        }

        auto map = objVal->get<Symbols::Value::ObjectMap>();
        auto it = map.find(propertyName_);
        if (it == map.end()) {
            throw Exception("Property '" + propertyName_ + "' not found in object", filename_, line_, column_);
        }

        // Return a copy of the value to ensure proper type handling
        return it->second;
    }

    std::string toString() const override { return objectExpr_->toString() + "->" + propertyName_; }

  private:
    std::unique_ptr<ExpressionNode> objectExpr_;
    std::string                     propertyName_;
    std::string                     filename_;
    int                             line_;
    size_t                          column_;
};

}  // namespace Interpreter

#endif  // INTERPRETER_MEMBER_EXPRESSION_NODE_HPP
