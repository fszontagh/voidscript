#ifndef INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP
#define INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/ReturnException.hpp"
#include "Symbols/ClassRegistry.hpp"
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
        // Evaluate target object
        Symbols::Value objVal = objectExpr_->evaluate(interpreter);
        if (objVal.getType() != Symbols::Variables::Type::OBJECT) {
            throw Exception("Attempted to call method: '" + methodName_ + "' on non-object", filename_, line_, column_);
        }
        const auto & objMap = std::get<Symbols::Value::ObjectMap>(objVal.get());
        // Extract class name from reserved field
        auto         it     = objMap.find("__class__");
        if (it == objMap.end() || it->second.getType() != Symbols::Variables::Type::STRING) {
            throw Exception("Object is missing class metadata for method: '" + methodName_ + "'", filename_, line_,
                            column_);
        }
        std::string className = it->second.get<std::string>();
        // Lookup method in registry
        auto &      registry  = Symbols::ClassRegistry::instance();
        if (!registry.hasMethod(className, methodName_)) {
            throw Exception("Method not found: " + methodName_ + " in class " + className, filename_, line_, column_);
        }
        // Build argument values
        std::vector<Symbols::Value> values;
        // First argument: 'this'
        values.push_back(objVal);
        for (const auto & argExpr : args_) {
            values.push_back(argExpr->evaluate(interpreter));
        }
        // Invoke as function: methodName with binding to class namespace
        // Find FunctionSymbol
        auto *                                   sc       = Symbols::SymbolContainer::instance();
        const std::string                        globalNs = sc->currentScopeName();
        const std::string                        classNs  = globalNs + "." + className;
        std::shared_ptr<Symbols::FunctionSymbol> funcSym;
        // Search in classNs.functions
        std::string                              fnSymNs = classNs + ".functions";
        auto                                     sym     = sc->get(fnSymNs, methodName_);
        if (sym && sym->getKind() == Symbols::Kind::Function) {
            funcSym = std::static_pointer_cast<Symbols::FunctionSymbol>(sym);
        } else {
            throw std::runtime_error("Function symbol not found for method '" + methodName_ + "'");
            throw Exception("Function symbol not found for method: '" + methodName_ + "'", filename_, line_, column_);
        }
        // Execute method
        // Enter method scope
        sc->enter(classNs + "." + methodName_);
        // Bind parameters: first parameter name 'this'? We assume method signature includes parameters after 'this'
        // 'this' variable name reserved as 'this'
        auto thisSym = Symbols::SymbolFactory::createVariable("this", objVal, classNs + "." + methodName_);
        sc->add(thisSym);
        const auto & params = funcSym->parameters();
        for (size_t i = 0; i < params.size(); ++i) {
            const auto & p      = params[i];
            auto   varSym = Symbols::SymbolFactory::createVariable(p.name, values[i + 1], classNs + "." + methodName_);
            sc->add(varSym);
        }
        // Run operations
        Symbols::SymbolContainer::instance();
        Symbols::SymbolContainer * dummy = sc;
        (void) dummy;
        Symbols::SymbolContainer * container = sc;
        for (const auto & op : Operations::Container::instance()->getAll(classNs + "." + methodName_)) {
            try {
                interpreter.runOperation(*op);
            } catch (const ReturnException & re) {
                // Exit scope
                sc->enterPreviousScope();
                return re.value();
            }
        }
        // Exit scope
        sc->enterPreviousScope();
        // No return: return null
        return Symbols::Value::makeNull();
    }

    std::string toString() const override { return std::string("MethodCall{ this->" + methodName_ + " }"); }
};

}  // namespace Interpreter

#endif  // INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP
