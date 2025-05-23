#ifndef INTERPRETER_METHOD_CALL_STATEMENT_NODE_HPP
#define INTERPRETER_METHOD_CALL_STATEMENT_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Modules/UnifiedModuleManager.hpp"
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
            auto currentScope = sc->currentScopeName();
            auto sym = sc->get(currentScope, targetObject_);
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

            // Build the full class scope name
            const std::string class_scope_name = currentScope + Symbols::SymbolContainer::SCOPE_SEPARATOR + className;

            // Look up method in class scope
            auto class_scope_table = sc->getScopeTable(class_scope_name);
            if (!class_scope_table) {
                throw Exception("Class " + className + " not found in scope " + class_scope_name, filename_, line_, column_);
            }

            auto sym_method = class_scope_table->get(Symbols::SymbolContainer::DEFAULT_FUNCTIONS_SCOPE, methodName_);
            if (!sym_method || sym_method->getKind() != Symbols::Kind::Function) {
                throw Exception("Method '" + methodName_ + "' not found in class " + className, filename_, line_, column_);
            }

            // Execute the method through the interpreter
            auto funcSym = std::static_pointer_cast<Symbols::FunctionSymbol>(sym_method);
            const auto& params = funcSym->parameters();
            
            // Create and enter method scope
            const std::string methodNs = class_scope_name + Symbols::SymbolContainer::SCOPE_SEPARATOR + methodName_;
            sc->enter(methodNs);

            // Bind 'this' and parameters
            sc->add(Symbols::SymbolFactory::createVariable("this", objValue, methodNs));
            for (size_t i = 0; i < params.size(); ++i) {
                sc->add(Symbols::SymbolFactory::createVariable(params[i].name, argValues[i], methodNs));
            }

            // Execute method body
            for (const auto& op : Operations::Container::instance()->getAll(methodNs)) {
                try {
                    interpreter.runOperation(*op);
                } catch (const ReturnException& re) {
                    // Allow return values to propagate
                    sc->enterPreviousScope();
                    throw;
                }
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
