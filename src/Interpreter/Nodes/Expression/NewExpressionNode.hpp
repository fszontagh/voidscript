#ifndef INTERPRETER_NEW_EXPRESSION_NODE_HPP
#define INTERPRETER_NEW_EXPRESSION_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Parser/ParsedExpression.hpp"
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
        auto & registry = Modules::UnifiedModuleManager::instance();
        // Ensure class is defined
        if (!registry.hasClass(className_)) {
            throw Exception("Class not found: " + className_, filename_, line_, column_);
        }
        // Initialize object fields from class definition
        const auto &              info = registry.getClassInfo(className_);
        Symbols::Value::ObjectMap obj;
        
        // First, initialize all properties with their default values
        for (const auto & prop : info.properties) {
            Symbols::Value value = Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
            if (prop.defaultValueExpr) {
                // Build and evaluate default expression
                auto exprNode = Parser::buildExpressionFromParsed(prop.defaultValueExpr);
                value = exprNode->evaluate(interpreter);
            }
            obj[prop.name] = value;
        }
        
        // Embed class metadata for method dispatch
        obj["__class__"] = Symbols::Value(className_);
        
        // Create the initial class instance
        Symbols::Value instance = Symbols::Value::makeClassInstance(obj);
        
        // Check if there's a constructor method
        bool hasConstructor = false;
        const Modules::ClassInfo::MethodInfo* constructorMethod = nullptr;
        for (const auto & method : info.methods) {
            if (method.name == "construct") {
                hasConstructor = true;
                constructorMethod = &method;
                break;
            }
        }
        
        if (hasConstructor) {
            // Call the constructor method
            std::vector<Symbols::Value> constructorArgs;
            constructorArgs.push_back(instance); // 'this' argument
            
            // Add the provided arguments
            for (const auto & arg : args_) {
                constructorArgs.push_back(arg->evaluate(interpreter));
            }
            
            // Call the constructor through UnifiedModuleManager
            std::string constructorName = className_ + "::construct";
            try {
                // Call the constructor function
                auto result = registry.callFunction(constructorName, constructorArgs);
                // Update our instance with any changes made by the constructor
                if (result.getType() == Symbols::Variables::Type::CLASS) {
                    instance = result;
                }
            } catch (const std::exception & e) {
                throw Exception(std::string("Constructor error: ") + e.what(), filename_, line_, column_);
            }
        } else if (!args_.empty()) {
            // No constructor but we have arguments - apply them to properties in order
            size_t propIndex = 0;
            for (const auto & prop : info.properties) {
                if (propIndex < args_.size()) {
                    Symbols::Value val = args_[propIndex]->evaluate(interpreter);
                    obj[prop.name] = val;
                    propIndex++;
                } else {
                    break;
                }
            }
            
            // Update the instance with the modified object map
            instance = Symbols::Value::makeClassInstance(obj);
        }
        
        return instance;
    }

    std::string toString() const override { return "NewExpression{ class=" + className_ + " }"; }
};

}  // namespace Interpreter

#endif  // INTERPRETER_NEW_EXPRESSION_NODE_HPP
