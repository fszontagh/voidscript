#ifndef DYNAMICMEMBEREXPRESSIONNODE_HPP
#define DYNAMICMEMBEREXPRESSIONNODE_HPP

#include <memory>
#include <string>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

class DynamicMemberExpressionNode : public ExpressionNode {
  public:
    DynamicMemberExpressionNode(std::unique_ptr<ExpressionNode> object, std::unique_ptr<ExpressionNode> memberExpr,
                                const std::string & filename, int line, size_t column) :
        ExpressionNode(),
        object_(std::move(object)),
        memberExpr_(std::move(memberExpr)),
        filename_(filename),
        line_(line),
        column_(column) {}

    Symbols::Value::ValuePtr evaluate(Interpreter & interpreter) const override {
        // Evaluate the object expression to get the object
        auto object = object_->evaluate(interpreter);
        if (object->getType() != Symbols::Variables::Type::OBJECT) {
            throw Exception("Cannot access member of non-object value", filename_, line_, column_);
        }

        // Evaluate the member expression to get the member name
        auto memberName = memberExpr_->evaluate(interpreter);
        if (memberName->getType() != Symbols::Variables::Type::STRING) {
            throw Exception("Member name must evaluate to a string", filename_, line_, column_);
        }

        // Get the member value
        std::string name = memberName->get<std::string>();
        if (name.empty()) {
            throw Exception("Member name cannot be empty", filename_, line_, column_);
        }

        // Remove quotes if present
        if (name.front() == '"' && name.back() == '"') {
            name = name.substr(1, name.size() - 2);
        }

        // Access the member using object's member map
        //const auto & map = std::get<Symbols::Value::ObjectMap>(object);
        auto map = object->get<Symbols::Value::ObjectMap>();
        auto it  = map.find(name);
        if (it == map.end()) {
            throw Exception("Member '" + name + "' not found in object", filename_, line_, column_);
        }
        return it->second;
    }

    std::string toString() const override { return object_->toString() + "->" + memberExpr_->toString(); }

  private:
    std::unique_ptr<ExpressionNode> object_;
    std::unique_ptr<ExpressionNode> memberExpr_;
    std::string                     filename_;
    int                             line_;
    size_t                          column_;
};

}  // namespace Interpreter

#endif  // DYNAMICMEMBEREXPRESSIONNODE_HPP
