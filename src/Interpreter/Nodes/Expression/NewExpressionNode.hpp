#ifndef INTERPRETER_NEW_EXPRESSION_NODE_HPP
#define INTERPRETER_NEW_EXPRESSION_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Parser/ParsedExpression.hpp"
#include "Symbols/ClassRegistry.hpp"
#include "Symbols/Value.hpp"

// Forward declaration for expression builder
namespace Parser {
std::unique_ptr<Interpreter::ExpressionNode> buildExpressionFromParsed(const ParsedExpressionPtr & expr);
}

namespace Interpreter {

/**
 * @brief AST node for 'new' expressions, instantiating objects of a class.
 */
class NewExpressionNode : public ExpressionNode {
    std::string                                  className_;
    std::vector<std::unique_ptr<ExpressionNode>> args_;
    std::string                                  filename_;
    int                                          line_;
    size_t                                       column_;

  public:
    NewExpressionNode(const std::string & className, std::vector<std::unique_ptr<ExpressionNode>> args,
                      const std::string & filename, int line, size_t column) :
        className_(className),
        args_(std::move(args)),
        filename_(filename),
        line_(line),
        column_(column) {}

    Symbols::Value evaluate(Interpreter & interpreter) const override {
        auto & registry = Symbols::ClassRegistry::instance();
        // Ensure class is defined
        if (!registry.hasClass(className_)) {
            throw Exception("Class not found: " + className_, filename_, line_, column_);
        }
        // Initialize object fields from class definition
        const auto &              info = registry.getClassInfo(className_);
        Symbols::Value::ObjectMap obj;
        // Default initialization for all properties
        size_t                    propCount = info.properties.size();
        for (size_t i = 0; i < propCount; ++i) {
            const auto &   prop  = info.properties[i];
            Symbols::Value value = Symbols::Value::makeNull();
            if (prop.defaultValueExpr) {
                // Build and evaluate default expression
                auto exprNode = Parser::buildExpressionFromParsed(prop.defaultValueExpr);
                value         = exprNode->evaluate(interpreter);
            }
            obj[prop.name] = value;
        }
        // Override with constructor arguments
        if (args_.size() > info.properties.size()) {
            throw Exception("Too many constructor arguments for class: " + className_, filename_, line_, column_);
        }
        for (size_t j = 0; j < args_.size(); ++j) {
            Symbols::Value val           = args_[j]->evaluate(interpreter);
            obj[info.properties[j].name] = val;
        }
        // Embed class metadata for method dispatch
        obj["__class__"] = Symbols::Value(className_);
        return Symbols::Value(obj);
    }

    std::string toString() const override { return "NewExpression{ class=" + className_ + " }"; }
};

}  // namespace Interpreter

#endif  // INTERPRETER_NEW_EXPRESSION_NODE_HPP
