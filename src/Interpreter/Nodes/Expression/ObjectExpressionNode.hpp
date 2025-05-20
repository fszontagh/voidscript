#ifndef INTERPRETER_OBJECT_EXPRESSION_NODE_HPP
#define INTERPRETER_OBJECT_EXPRESSION_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

class ObjectExpressionNode : public ExpressionNode {
  public:
    using ObjectMap = Symbols::ObjectMap;

    explicit ObjectExpressionNode(std::vector<std::pair<std::string, std::unique_ptr<ExpressionNode>>> members) :
        members_(std::move(members)) {}

    Symbols::ValuePtr evaluate(Interpreter & interpreter, std::string /*filename*/, int /*line*/,
                               size_t /*col */) const override {
        ObjectMap obj;
        for (const auto & kv : members_) {
            obj[kv.first] = kv.second->evaluate(interpreter);
        }
        return obj;
    }

    std::string toString() const override { return "[object]"; }

  private:
    std::vector<std::pair<std::string, std::unique_ptr<ExpressionNode>>> members_;
};

}  // namespace Interpreter

#endif  // INTERPRETER_OBJECT_EXPRESSION_NODE_HPP
