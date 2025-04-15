#ifndef INTERPRETER_OPERATION_HPP
#define INTERPRETER_OPERATION_HPP

#include <cstdint>

#include <memory>
#include <string>

#include "ExpressionNode.hpp"

namespace Interpreter {
enum OperationType : std::uint8_t {
    Assignment,
    Expression,

};

struct Operation {
    OperationType type;

    // Általános mezők
    std::string targetVariable;
    std::unique_ptr<ExpressionNode> expression;

    Operation(OperationType t, std::string var, std::unique_ptr<ExpressionNode> expr)
        : type(t), targetVariable(std::move(var)), expression(std::move(expr)) {}
};


};  // namespace Interpreter
#endif
