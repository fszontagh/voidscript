#ifndef INTERPRETER_NEW_EXPRESSION_NODE_HPP
#define INTERPRETER_NEW_EXPRESSION_NODE_HPP

#include <iostream>  // Required for std::cerr
#include <memory>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/ReturnException.hpp"
#include "Parser/ParsedExpression.hpp"

#include "Symbols/FunctionSymbol.hpp"
#include "Symbols/SymbolContainer.hpp"
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
    NewExpressionNode(const std::string & className, std::vector<std::unique_ptr<ExpressionNode>> && args,
                      const std::string & filename, int line, size_t column) :
        className_(className),
        args_(std::move(args)),
        filename_(filename),
        line_(line),
        column_(column) {}

    Symbols::ValuePtr evaluate(class Interpreter & interpreter, std::string filename = "", int line = 0,
                               size_t column = 0) const override {
        auto                                     sc            = Symbols::SymbolContainer::instance();
        std::map<std::string, Symbols::ValuePtr> objProperties;

        // First try to find the class info with the name as provided
        std::string       fqClassName = className_;
        bool              classFound  = sc->hasClass(fqClassName);
        const std::string classNs = sc->currentScopeName() + Symbols::SymbolContainer::SCOPE_SEPARATOR + fqClassName;

        if (!classFound) {
            throw Exception("Class not found: " + className_, filename_, line_, column_);
        }

        // Now get the class properties from symbol container
        try {
            const auto & classInfo = sc->getClassInfo(fqClassName);

            // Initialize properties with default values from class definition
            for (const auto & prop : classInfo.properties) {
                Symbols::ValuePtr value;
                if (prop.defaultValueExpr) {
                    try {
                        // Try to evaluate default value expression if it exists
                        auto exprNode = Parser::buildExpressionFromParsed(prop.defaultValueExpr);
                        value         = exprNode->evaluate(interpreter);
                    } catch (const Exception & e) {
                        // If evaluation fails, use appropriate default value instead of null
                        switch (prop.type) {
                            case Symbols::Variables::Type::INTEGER:
                                value = Symbols::ValuePtr(0);
                                break;
                            case Symbols::Variables::Type::DOUBLE:
                                value = Symbols::ValuePtr(0.0);
                                break;
                            case Symbols::Variables::Type::FLOAT:
                                value = Symbols::ValuePtr(0.0f);
                                break;
                            case Symbols::Variables::Type::STRING:
                                value = Symbols::ValuePtr("");
                                break;
                            case Symbols::Variables::Type::BOOLEAN:
                                value = Symbols::ValuePtr(false);
                                break;
                            case Symbols::Variables::Type::OBJECT:
                                value = Symbols::ValuePtr(Symbols::ObjectMap());
                                break;
                            default:
                                value = Symbols::ValuePtr::null(prop.type);
                                break;
                        }
                    }
                } else {
                    // Create default value for the property type instead of null
                    switch (prop.type) {
                        case Symbols::Variables::Type::INTEGER:
                            value = Symbols::ValuePtr(0);
                            break;
                        case Symbols::Variables::Type::DOUBLE:
                            value = Symbols::ValuePtr(0.0);
                            break;
                        case Symbols::Variables::Type::FLOAT:
                            value = Symbols::ValuePtr(0.0f);
                            break;
                        case Symbols::Variables::Type::STRING:
                            value = Symbols::ValuePtr("");
                            break;
                        case Symbols::Variables::Type::BOOLEAN:
                            value = Symbols::ValuePtr(false);
                            break;
                        case Symbols::Variables::Type::OBJECT:
                            value = Symbols::ValuePtr(Symbols::ObjectMap());
                            break;
                        default:
                            value = Symbols::ValuePtr::null(prop.type);
                            break;
                    }
                }
                objProperties[prop.name] = value;
            }
        } catch (const std::exception & e) {
            // Continue with empty properties if class info not available
        }

        // Create object using ValuePtr constructor and set type to CLASS immediately
        Symbols::ValuePtr newObject = Symbols::ValuePtr::null(Symbols::Variables::Type::CLASS);

        // Set the properties
        for (const auto & [key, value] : objProperties) {
            newObject[key] = value;
        }

        // Add class name property to the object to identify its class type
        newObject["$class_name"] = Symbols::ValuePtr(className_);

        // Check if class has a constructor registered
        bool hasClassInfo = sc->hasClass(fqClassName);
        
        if (hasClassInfo) {
            // Check for common constructor method names
            std::vector<std::string> constructorNames = { "constructor", "construct", "__construct" };
            std::string              foundConstructor;

            for (const auto & constructorName : constructorNames) {
                if (sc->hasMethod(fqClassName, constructorName)) {
                    foundConstructor = constructorName;
                    break;
                }
            }

            if (!foundConstructor.empty()) {
                // Has constructor - prepare to call it

                // Evaluate the arguments
                std::vector<Symbols::ValuePtr> evaluatedArgs;
                for (const auto & argExpr : args_) {
                    evaluatedArgs.push_back(argExpr->evaluate(interpreter));
                }

                // Get function symbol for constructor using the correct namespace
                std::string constructorFullName =
                    classNs + Symbols::SymbolContainer::SCOPE_SEPARATOR + foundConstructor;
                std::shared_ptr<Symbols::FunctionSymbol> constructorSymbol =
                    std::dynamic_pointer_cast<Symbols::FunctionSymbol>(sc->get(classNs, foundConstructor));
                if (constructorSymbol) {
                    // Fill missing arguments with type-appropriate defaults
                    const auto & params = constructorSymbol->parameters();
                    if (evaluatedArgs.size() > params.size()) {
                        throw Exception("Argument count mismatch for constructor '" + foundConstructor +
                                            "' of class '" + className_ + "'. Expected " +
                                            std::to_string(params.size()) + ", got " +
                                            std::to_string(evaluatedArgs.size()),
                                        filename_, line_, column_);
                    }

                    // Validate argument types
                    for (size_t i = 0; i < evaluatedArgs.size(); ++i) {
                        if (params[i].type != Symbols::Variables::Type::UNDEFINED_TYPE &&
                            params[i].type != evaluatedArgs[i].getType() &&
                            evaluatedArgs[i].getType() != Symbols::Variables::Type::NULL_TYPE) {
                            throw Exception("Argument type mismatch for parameter '" + params[i].name +
                                                "' of constructor '" + foundConstructor + "' in class '" + className_ +
                                                "'. Expected " + Symbols::Variables::TypeToString(params[i].type) +
                                                ", got " + Symbols::Variables::TypeToString(evaluatedArgs[i].getType()),
                                            filename_, line_, column_);
                        }
                    }

                    // Create a new scope for the constructor call
                    std::string callScope = sc->enterFunctionCallScope(foundConstructor);

                    // Add "this" to the current scope
                    sc->addVariable(Symbols::SymbolFactory::createVariable("this", newObject, callScope));

                    try {
                        // Since FunctionSymbol doesn't have direct access to operations,
                        // we need to execute the constructor function in a different way

                        // Add parameters to scope
                        const auto & params = constructorSymbol->parameters();
                        for (size_t i = 0; i < params.size(); ++i) {
                            sc->addVariable(
                                Symbols::SymbolFactory::createVariable(params[i].name, evaluatedArgs[i], callScope));
                        }

                        // Get the operations associated with this constructor from the operations container
                        const auto & operations = Operations::Container::instance()->getAll(constructorFullName);

                        // Execute each operation
                        for (const auto & op : operations) {
                            interpreter.runOperation(*op);
                        }

                    } catch (const ReturnException & re) {
                        // Constructor return value is ignored
                    } catch (...) {
                        sc->enterPreviousScope();  // Ensure we pop the scope
                        throw;
                    }

                    // Exit the constructor scope
                    sc->enterPreviousScope();

                } else {
                    //  throw Exception(
                    //      "Constructor '" + moduleClassInfo.constructorName + "' not found for class '" + className_ + "'",
                    //      filename_, line_, column_);
                }

            } else if (!args_.empty()) {
                // No constructor but arguments provided
                throw Exception("Class '" + className_ + "' does not have a constructor, but arguments were provided.",
                                filename_, line_, column_);
            }

            return newObject;
        }
        
        // Return the object even if no module class info
        return newObject;
    }

    std::string toString() const override {
        std::string result = "NewExpressionNode[class=" + className_ + ", args=[";
        for (size_t i = 0; i < args_.size(); ++i) {
            result += args_[i]->toString();
            if (i < args_.size() - 1) {
                result += ", ";
            }
        }
        result += "]]";
        return result;
    }
};
};  // namespace Interpreter
#endif