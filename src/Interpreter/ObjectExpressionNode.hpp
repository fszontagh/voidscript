#ifndef INTERPRETER_OBJECT_EXPRESSION_NODE_HPP
#define INTERPRETER_OBJECT_EXPRESSION_NODE_HPP

#include "ExpressionNode.hpp"
#include "Symbols/Value.hpp"
#include <vector>
#include <string>
#include <memory>

namespace Interpreter {

class ObjectExpressionNode : public ExpressionNode {
  public:
    using ObjectMap = Symbols::Value::ObjectMap;

    explicit ObjectExpressionNode(std::vector<std::pair<std::string, std::unique_ptr<ExpressionNode>>> members)
        : members_(std::move(members)) {}

    Symbols::Value evaluate(Interpreter & interpreter) const override {
        ObjectMap obj;
        for (const auto &kv : members_) {
            obj[kv.first] = kv.second->evaluate(interpreter);
        }
        return Symbols::Value(obj);
    }

    std::string toString() const override { return "[object]"; }

  private:
    std::vector<std::pair<std::string, std::unique_ptr<ExpressionNode>>> members_;
};

} // namespace Interpreter

#endif // INTERPRETER_OBJECT_EXPRESSION_NODE_HPP