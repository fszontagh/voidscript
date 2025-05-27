#ifndef INTERPRETER_METHOD_CALL_STATEMENT_NODE_HPP
#define INTERPRETER_METHOD_CALL_STATEMENT_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

/**
 * @brief Statement node for handling method calls on objects.
 *   e.g., $obj->method() or $obj->method($arg1, $arg2)
 */
class MethodCallStatementNode : public StatementNode {
private:
    std::string targetObject_;                                 // The name of the object to call method on
    std::string methodName_;                                   // The name of the method to call
    std::vector<std::unique_ptr<ExpressionNode>> arguments_;  // Method arguments

public:
    MethodCallStatementNode(std::string targetObj,
                           std::string methodName,
                           std::vector<std::unique_ptr<ExpressionNode>> args,
                           const std::string& fileName,
                           int line,
                           size_t col)
        : StatementNode(fileName, line, col)
        , targetObject_(std::move(targetObj))
        , methodName_(std::move(methodName))
        , arguments_(std::move(args)) {}

    void interpret(Interpreter& interpreter) const override {
        try {
            // Evaluate arguments
            std::vector<Symbols::ValuePtr> argValues;
            argValues.reserve(arguments_.size());
            for (const auto& expr : arguments_) {
                argValues.push_back(expr->evaluate(interpreter));
            }

            // Get the target object from the symbol container in the current scope
            auto* sc = Symbols::SymbolContainer::instance();
            auto sym = sc->get(sc->currentScopeName(), targetObject_);
            if (!sym) {
                throw Exception("Object not found: " + targetObject_, filename_, line_, column_);
            }

            // Get the object value
            auto objValue = sym->getValue();
            if (objValue.getType() != Symbols::Variables::Type::OBJECT && 
                objValue.getType() != Symbols::Variables::Type::CLASS) {
                throw Exception("Cannot call method on non-object/non-class value: " + targetObject_, filename_, line_, column_);
            }

            // Get class name from object metadata
            const auto& objMap = objValue.get<Symbols::ObjectMap>();
            auto it = objMap.find("$class_name");
            if (it == objMap.end()) {
                throw Exception("Object is missing class metadata for method: " + methodName_, filename_, line_, column_);
            }
            std::string className = it->second.get<std::string>();

            // Build possible class scope names to search for the method
            std::vector<std::string> class_scope_names;
            
            // Try multiple possible scope paths
            // 1. Current scope + class name
            const std::string current_scope = sc->currentScopeName();
            class_scope_names.push_back(current_scope + Symbols::SymbolContainer::SCOPE_SEPARATOR + className);
            
            // 2. Just the class name (for global classes)
            class_scope_names.push_back(className);
            
            // 3. File scope + class name (traditional approach)
            class_scope_names.push_back(filename_ + Symbols::SymbolContainer::SCOPE_SEPARATOR + className);
            
            // Look up method in potential class scopes
            // Check if method exists in class using the same approach as MethodCallExpressionNode
            if (!sc->hasMethod(className, methodName_)) {
                throw Exception("Method '" + methodName_ + "' not found in class " + className, filename_, line_, column_);
            }
            
            // Get the method symbol using findMethod
            std::shared_ptr<Symbols::Symbol> sym_method = sc->findMethod(className, methodName_);
            std::string resolved_class_scope;
            
            if (sym_method) {
                // Found via proper method lookup, resolve the class scope
                resolved_class_scope = sc->findClassNamespace(className);
            } else {
                // This shouldn't happen if hasMethod returned true, but fallback anyway
                throw Exception("Method '" + methodName_ + "' not found in class " + className, filename_, line_, column_);
            }
            
            if (!sym_method) {
                throw Exception("Method '" + methodName_ + "' not found in class " + className, filename_, line_, column_);
            }

            // Execute the method through the interpreter
            std::shared_ptr<Symbols::FunctionSymbol> funcSym;
            
            // Both Function and Method can be cast to FunctionSymbol since Method inherits from Function
            if (sym_method->getKind() == Symbols::Kind::Method) {
                funcSym = std::dynamic_pointer_cast<Symbols::FunctionSymbol>(sym_method);
            } else {
                funcSym = std::static_pointer_cast<Symbols::FunctionSymbol>(sym_method);
            }
            
            const auto& params = funcSym->parameters();
            
            // Create and enter method scope
            const std::string methodNs = resolved_class_scope + Symbols::SymbolContainer::SCOPE_SEPARATOR + methodName_;
            
            // Create a new scope for method execution
            sc->create(methodNs);
            
            // Set up the method context
            interpreter.setThisObject(objValue);
            
            // Bind 'this' and parameters
            sc->addVariable(Symbols::SymbolFactory::createVariable("this", objValue, methodNs));
            
            // Add parameters
            for (size_t i = 0; i < std::min(params.size(), argValues.size()); ++i) {
                sc->addVariable(Symbols::SymbolFactory::createVariable(params[i].name, argValues[i], methodNs));
            }

            // Execute method body
            try {
                for (const auto& op : Operations::Container::instance()->getAll(methodNs)) {
                    try {
                        interpreter.runOperation(*op);
                    } catch (const ReturnException& re) {
                        // Clean up
                        interpreter.clearThisObject();
                        sc->enterPreviousScope();
                        // Allow return values to propagate
                        throw;
                    }
                }
            } catch (...) {
                // Make sure we clean up even on exceptions
                interpreter.clearThisObject();
                sc->enterPreviousScope();
                throw;
            }

            // Exit method scope
            sc->enterPreviousScope();

        } catch (const Exception&) {
            throw;
        } catch (const std::exception& e) {
            throw Exception(e.what(), filename_, line_, column_);
        }
    }

    std::string toString() const override {
        return "MethodCall: " + targetObject_ + "->" + methodName_ + "(...)";
    }
};

} // namespace Interpreter

#endif // INTERPRETER_METHOD_CALL_STATEMENT_NODE_HPP
