#ifndef INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP
#define INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/ReturnException.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

/**
 * @brief Expression node for invoking class methods via object->method(...).
 */
class MethodCallExpressionNode : public ExpressionNode {
    std::unique_ptr<ExpressionNode>              objectExpr_;
    std::string                                  methodName_;
    std::vector<std::unique_ptr<ExpressionNode>> args_;
    std::string                                  filename_;
    int                                          line_;
    size_t                                       column_;

  public:
    MethodCallExpressionNode(std::unique_ptr<ExpressionNode> objectExpr, std::string methodName,
                             std::vector<std::unique_ptr<ExpressionNode>> args, const std::string & filename, int line,
                             size_t column) :
        objectExpr_(std::move(objectExpr)),
        methodName_(std::move(methodName)),
        args_(std::move(args)),
        filename_(filename),
        line_(line),
        column_(column) {}

    Symbols::Value evaluate(Interpreter & interpreter) const override {
        using namespace Symbols;
        // Evaluate target object and track original symbol for write-back
        auto *                           sc      = Symbols::SymbolContainer::instance();
        // Determine original variable symbol (only simple identifiers)
        std::string                      fileNs  = sc->currentScopeName();
        std::string                      varNs   = fileNs + "::variables";
        std::string                      objName = objectExpr_->toString();
        std::shared_ptr<Symbols::Symbol> origSym;
        if (sc->exists(objName, varNs)) {
            origSym = sc->get(varNs, objName);
        }
        try {
            // Evaluate target object (produces a copy)
            Value objVal = objectExpr_->evaluate(interpreter);

            // Allow method calls on class instances (and plain objects with class metadata)
            if (objVal.getType() != Variables::Type::OBJECT && objVal.getType() != Variables::Type::CLASS) {
                throw Exception("Attempted to call method: '" + methodName_ + "' on non-object", filename_, line_,
                                column_);
            }
            const auto & objMap = std::get<Value::ObjectMap>(objVal.get());

            // Extract class name
            auto it = objMap.find("__class__");
            if (it == objMap.end() || it->second.getType() != Variables::Type::STRING) {
                throw std::invalid_argument("Object is missing class metadata for method: " + methodName_);
                //throw Exception("Object is missing class metadata for method: '" + methodName_ + "'", filename_, line_,                                column_);
            }
            std::string className = it->second.get<std::string>();
            // Verify method exists
            auto &      registry  = Modules::UnifiedModuleManager::instance();
            if (!registry.hasMethod(className, methodName_)) {
                throw Exception("Method not found: " + methodName_ + " in class " + className, filename_, line_,
                                column_);
            }

            // Evaluate arguments (including 'this')
            std::vector<Value> argValues;
            argValues.reserve(args_.size() + 1);
            argValues.push_back(objVal);
            for (const auto & arg : args_) {
                argValues.push_back(arg->evaluate(interpreter));
            }
            // Native method override via UnifiedModuleManager
            {
                auto &      mgr      = Modules::UnifiedModuleManager::instance();
                std::string fullName = className + "::" + methodName_;
                if (mgr.hasFunction(fullName)) {
                    Value           ret     = mgr.callFunction(fullName, argValues);
                    Variables::Type retType = mgr.getFunctionReturnType(fullName);
                    // Write back modified object if returned
                    if (origSym &&
                        (ret.getType() == Variables::Type::OBJECT || ret.getType() == Variables::Type::CLASS)) {
                        if (origSym->getValue().getType() == ret.getType()) {
                            origSym->setValue(ret);
                        }
                        //origSym->setValue(ret);
                    }
                    return ret;
                    //return mgr.getFunctionNullValue(fullName);
                    //return Value::makeNull(Variables::Type::NULL_TYPE);
                }
            }

            // Locate function symbol in class namespace
            auto *            sc_instance = SymbolContainer::instance();  // Renamed to avoid conflict with outer sc
            // Lookup method symbol in class namespace.
            // classNs is the scope where the class's methods are defined.
            // This often takes the form: current_file_scope_name + "::" + className
            const std::string current_file_scope =
                sc_instance->currentScopeName();  // Or derive from filename if more reliable
            const std::string class_scope_name = current_file_scope + "::" + className;

            Symbols::SymbolPtr sym               = nullptr;
            auto               class_scope_table = sc_instance->getScopeTable(class_scope_name);
            if (class_scope_table) {
                sym = class_scope_table->get(Symbols::SymbolContainer::DEFAULT_FUNCTIONS_SCOPE, methodName_);
            }

            if (!sym || sym->getKind() != Kind::Function) {
                throw Exception("Function symbol not found for method: '" + methodName_ + "' in class scope '" +
                                    class_scope_name + "'",
                                filename_, line_, column_);
            }
            auto            funcSym    = std::static_pointer_cast<FunctionSymbol>(sym);
            Variables::Type returnType = funcSym->returnType();
            // Check argument count
            const auto &    params     = funcSym->parameters();
            if (params.size() != args_.size()) {
                throw Exception("Method '" + methodName_ + "' expects " + std::to_string(params.size()) +
                                    " args, got " + std::to_string(args_.size()),
                                filename_, line_, column_);
            }
            // Enter method scope under class namespace
            const std::string methodNs = class_scope_name + "::" + methodName_;
            sc_instance->enter(methodNs);
            // Bind 'this'
            sc_instance->add(SymbolFactory::createVariable("this", objVal, methodNs));
            // Bind parameters
            for (size_t i = 0; i < params.size(); ++i) {
                sc_instance->add(SymbolFactory::createVariable(params[i].name, argValues[i + 1], methodNs));
            }
            // Execute method body
            for (const auto & op : Operations::Container::instance()->getAll(methodNs)) {
                try {
                    interpreter.runOperation(*op);
                } catch (const ReturnException & re) {
                    // Write back modified object state before returning
                    if (origSym) {
                        auto method_scope_table = sc_instance->getScopeTable(methodNs);
                        if (method_scope_table) {
                            auto thisSym =
                                method_scope_table->get(Symbols::SymbolContainer::DEFAULT_VARIABLES_SCOPE, "this");
                            if (thisSym) {
                                origSym->setValue(thisSym->getValue());
                            } else {
                                // Log error or handle: 'this' not found in method scope where expected
                            }
                        } else {
                            // Log error or handle: method scope table not found
                        }
                    }
                    sc_instance->enterPreviousScope();
                    return re.value();
                }
            }
            // Write back modified object state after method execution
            if (origSym) {
                auto method_scope_table = sc_instance->getScopeTable(methodNs);
                if (method_scope_table) {
                    auto thisSym = method_scope_table->get(Symbols::SymbolContainer::DEFAULT_VARIABLES_SCOPE, "this");
                    if (thisSym) {
                        origSym->setValue(thisSym->getValue());
                    } else {
                        // Log error or handle: 'this' not found in method scope where expected
                    }
                } else {
                    // Log error or handle: method scope table not found
                }
            }
            // Exit method scope
            sc_instance->enterPreviousScope();
            // Return type checking: if method declares a non-null return type, error if no value was returned
            if (returnType == Variables::Type::NULL_TYPE) {
                return Value::makeNull(Variables::Type::NULL_TYPE);
            } else {
                throw Exception("Method '" + methodName_ + "' (return type: " + Variables::TypeToString(returnType) +
                                    ") did not return a value",
                                filename_, line_, column_);
            }
        } catch (const std::exception & e) {
            throw Exception(e.what(), filename_, line_, column_);
        }
    }

    std::string toString() const override { return std::string("MethodCall{ this->" + methodName_ + " }"); }
};

}  // namespace Interpreter

#endif  // INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP
