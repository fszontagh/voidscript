#ifndef INTERPRETER_CLASS_DEFINITION_STATEMENT_NODE_HPP
#define INTERPRETER_CLASS_DEFINITION_STATEMENT_NODE_HPP

#include <string>
#include <vector>

#include "Interpreter/Interpreter.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Interpreter/ReturnException.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"
#include "Interpreter/ExpressionBuilder.hpp"
#include "Parser/ParsedExpression.hpp"

namespace Interpreter {

/**
 * @brief AST node representing a class definition statement.
 */
class ClassDefinitionStatementNode : public StatementNode {
    std::string                                   className_;
    std::vector<Modules::ClassInfo::PropertyInfo> privateProperties_;
    std::vector<Modules::ClassInfo::PropertyInfo> publicProperties_;
    std::vector<Modules::ClassInfo::MethodInfo>   publicMethods_;
    std::vector<Modules::ClassInfo::MethodInfo>   privateMethods_;

  public:
    ClassDefinitionStatementNode(const std::string &                           className,
                                 std::vector<Modules::ClassInfo::PropertyInfo> privateProps,
                                 std::vector<Modules::ClassInfo::PropertyInfo> publicProps,
                                 std::vector<Modules::ClassInfo::MethodInfo>   publicMethods,
                                 std::vector<Modules::ClassInfo::MethodInfo>   privateMethods,
                                 const std::string & filename, int line,
                                 size_t column) :
        StatementNode(filename, line, column),
        className_(className),
        privateProperties_(std::move(privateProps)),
        publicProperties_(std::move(publicProps)),
        publicMethods_(std::move(publicMethods)),
        privateMethods_(std::move(privateMethods)) {}

    // Helper method to build an expression node from a parsed expression
    std::unique_ptr<ExpressionNode> buildExpressionFromParsed(const std::shared_ptr<Parser::ParsedExpression>& expr) const {
        if (!expr) {
            return nullptr;
        }
        return Parser::buildExpressionFromParsed(expr);
    }

    void interpret(Interpreter & interpreter) const override {
        auto * sc = Symbols::SymbolContainer::instance();
        // Register class and its members in class registry
        auto & registry = Modules::UnifiedModuleManager::instance();
        // Register the class itself
        registry.registerClass(className_);
        
        interpreter.debugLog("Registering class: " + className_);
        
        // Register all properties in the registry
        for (const auto & prop : privateProperties_) {
            registry.registerProperty(className_, prop.name, prop.type, false); // isPublic = false for private properties
            interpreter.debugLog("Registered private property: " + prop.name + " in class " + className_);
        }
        
        for (const auto & prop : publicProperties_) {
            registry.registerProperty(className_, prop.name, prop.type, true); // isPublic = true for public properties
            interpreter.debugLog("Registered public property: " + prop.name + " in class " + className_);
        }
        
        // Create a class namespace for method definitions
        std::string currentScope = sc->currentScopeName();
        std::string classNs = currentScope + "::" + className_;
        
        // Create class scope if it doesn't exist
        if (!sc->scopeExists(classNs)) {
            sc->create(classNs);
            interpreter.debugLog("Created class scope: " + classNs);
        }
        
        // Create a dummy 'this' object for use during class definition
        Symbols::Value::ObjectMap dummyObj;
        
        // Add all properties to the dummy object
        for (const auto & prop : privateProperties_) {
            Symbols::Value defaultValue;
            if (prop.defaultValueExpr) {
                try {
                    auto expr = buildExpressionFromParsed(prop.defaultValueExpr);
                    if (expr) {
                        defaultValue = expr->evaluate(interpreter);
                    }
                } catch (const std::exception & e) {
                    interpreter.debugLog(std::string("Error evaluating default value: ") + e.what());
                    defaultValue = Symbols::Value::makeDefaultForType(prop.type);
                }
            } else {
                defaultValue = Symbols::Value::makeDefaultForType(prop.type);
            }
            dummyObj[prop.name] = defaultValue;
        }
        
        for (const auto & prop : publicProperties_) {
            Symbols::Value defaultValue;
            if (prop.defaultValueExpr) {
                try {
                    auto expr = buildExpressionFromParsed(prop.defaultValueExpr);
                    if (expr) {
                        defaultValue = expr->evaluate(interpreter);
                    }
                } catch (const std::exception & e) {
                    interpreter.debugLog(std::string("Error evaluating default value: ") + e.what());
                    defaultValue = Symbols::Value::makeDefaultForType(prop.type);
                }
            } else {
                defaultValue = Symbols::Value::makeDefaultForType(prop.type);
            }
            dummyObj[prop.name] = defaultValue;
        }
        
        // Add class metadata
        dummyObj["__class__"] = Symbols::Value(className_);
        
        // Create the dummy 'this' object
        Symbols::Value dummyThis = Symbols::Value::makeClassInstance(dummyObj);
        
        // Register all methods in the registry
        for (const auto & method : publicMethods_) {
            // Special handling for constructor
            if (method.name == "construct") {
                // Register constructor as both a method and a function
                registry.registerMethod(className_, method.name, Symbols::Variables::Type::CLASS, true);
                std::string constructorName = className_ + "::construct";
                registry.registerFunction(constructorName, [](const std::vector<Symbols::Value> & args) -> Symbols::Value {
                    // The first argument is 'this', which we'll modify and return
                    if (args.empty()) {
                        throw std::runtime_error("Constructor called without 'this' argument");
                    }
                    return args[0]; // Return the modified 'this' object
                }, Symbols::Variables::Type::CLASS);
            } else {
                registry.registerMethod(className_, method.name, method.returnType, true);
            }
            interpreter.debugLog("Registered public method: " + method.name + " in class " + className_);
        }
        
        for (const auto & method : privateMethods_) {
            // Special handling for constructor
            if (method.name == "construct") {
                // Register constructor as both a method and a function
                registry.registerMethod(className_, method.name, Symbols::Variables::Type::CLASS, false);
                std::string constructorName = className_ + "::construct";
                registry.registerFunction(constructorName, [](const std::vector<Symbols::Value> & args) -> Symbols::Value {
                    // The first argument is 'this', which we'll modify and return
                    if (args.empty()) {
                        throw std::runtime_error("Constructor called without 'this' argument");
                    }
                    return args[0]; // Return the modified 'this' object
                }, Symbols::Variables::Type::CLASS);
            } else {
                registry.registerMethod(className_, method.name, method.returnType, false);
            }
            interpreter.debugLog("Registered private method: " + method.name + " in class " + className_);
        }
        
        // Create a 'this' variable in each method scope
        for (const auto & method : publicMethods_) {
            std::string methodScope = classNs + "::" + method.name;
            if (!sc->scopeExists(methodScope)) {
                sc->create(methodScope);
            }
            // Add 'this' to the method scope
            auto thisSymbol = Symbols::SymbolFactory::createVariable("this", dummyThis, methodScope);
            sc->add(thisSymbol);
            interpreter.debugLog("Added 'this' variable to method scope: " + methodScope);
        }
        
        for (const auto & method : privateMethods_) {
            std::string methodScope = classNs + "::" + method.name;
            if (!sc->scopeExists(methodScope)) {
                sc->create(methodScope);
            }
            // Add 'this' to the method scope
            auto thisSymbol = Symbols::SymbolFactory::createVariable("this", dummyThis, methodScope);
            sc->add(thisSymbol);
            interpreter.debugLog("Added 'this' variable to method scope: " + methodScope);
        }
        
        // Execute all operations in the class namespace to register methods and their bodies
        auto ops = Operations::Container::instance()->getAll(classNs);
        for (const auto & op : ops) {
            try {
                interpreter.runOperation(*op);
            } catch (const ReturnException & re) {
                // Ignore return exceptions during class initialization
                interpreter.debugLog(std::string("Caught return exception during class initialization in: ") + className_);
            } catch (const std::exception & e) {
                interpreter.debugLog(std::string("Error executing operation in class scope: ") + e.what());
            }
        }
    }

    std::string toString() const override { return "ClassDefinition{ class=" + className_ + " }"; }
};

}  // namespace Interpreter

#endif  // INTERPRETER_CLASS_DEFINITION_STATEMENT_NODE_HPP
