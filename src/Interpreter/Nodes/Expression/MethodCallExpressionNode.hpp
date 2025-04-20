#ifndef INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP
#define INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/ReturnException.hpp"
// Access to recorded operations for class methods (method bodies)
#include "Interpreter/OperationContainer.hpp"
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
        using namespace Symbols;
        // Evaluate target object and track original symbol for write-back
        auto * sc = Symbols::SymbolContainer::instance();
        // Determine original variable symbol (only simple identifiers)
        std::string fileNs = sc->currentScopeName();
        std::string varNs  = fileNs + "::variables";
        std::string objName = objectExpr_->toString();
        std::shared_ptr<Symbols::Symbol> origSym;
        if (sc->exists(objName, varNs)) {
            origSym = sc->get(varNs, objName);
        }
        try {
            // Evaluate target object (produces a copy)
            Value objVal = objectExpr_->evaluate(interpreter);
            // Allow method calls on class instances (and plain objects with class metadata)
            if (objVal.getType() != Variables::Type::OBJECT &&
                objVal.getType() != Variables::Type::CLASS) {
                throw Exception("Attempted to call method: '" + methodName_ + "' on non-object", filename_, line_,
                                column_);
            }
            const auto & objMap = std::get<Value::ObjectMap>(objVal.get());
            // Extract class name
            auto         it     = objMap.find("__class__");
            if (it == objMap.end() || it->second.getType() != Variables::Type::STRING) {
                throw Exception("Object is missing class metadata for method: '" + methodName_ + "'", filename_, line_,
                                column_);
            }
            std::string className = it->second.get<std::string>();
            // Verify method exists
            auto &      registry  = ClassRegistry::instance();
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
            // Locate function symbol in class namespace
            auto *            sc       = SymbolContainer::instance();
            // Lookup method symbol in class namespace
            const std::string fileNs   = sc->currentScopeName();
            const std::string classNs  = fileNs + "::" + className;
            const std::string fnSymNs  = classNs + "::functions";
            auto              sym      = sc->get(fnSymNs, methodName_);
            if (!sym || sym->getKind() != Kind::Function) {
                throw Exception("Function symbol not found for method: '" + methodName_ + "' NS: " + fnSymNs, filename_,
                                line_, column_);
            }
            auto         funcSym = std::static_pointer_cast<FunctionSymbol>(sym);
            // Check argument count
            const auto & params  = funcSym->parameters();
            if (params.size() != args_.size()) {
                throw Exception("Method '" + methodName_ + "' expects " + std::to_string(params.size()) +
                                    " args, got " + std::to_string(args_.size()),
                                filename_, line_, column_);
            }
            // Enter method scope under class namespace
            const std::string methodNs = classNs + "::" + methodName_;
            sc->enter(methodNs);
            // Bind 'this'
            sc->add(SymbolFactory::createVariable("this", objVal, methodNs));
            // Bind parameters
            for (size_t i = 0; i < params.size(); ++i) {
                sc->add(SymbolFactory::createVariable(params[i].name, argValues[i + 1], methodNs));
            }
            // Execute method body
            for (const auto & op : Operations::Container::instance()->getAll(methodNs)) {
                try {
                    interpreter.runOperation(*op);
                } catch (const ReturnException & re) {
                    // Write back modified object state before returning
                    if (origSym) {
                        auto thisSym = sc->get(methodNs + "::variables", "this");
                        origSym->setValue(thisSym->getValue());
                    }
                    sc->enterPreviousScope();
                    return re.value();
                }
            }
            // Write back modified object state after method execution
            if (origSym) {
                auto thisSym = sc->get(methodNs + "::variables", "this");
                origSym->setValue(thisSym->getValue());
            }
            // Exit method scope
            sc->enterPreviousScope();
            return Value::makeNull();
        } catch (const std::exception & e) {
            throw Exception(e.what(), filename_, line_, column_);
        }
    }

    std::string toString() const override { return std::string("MethodCall{ this->" + methodName_ + " }"); }
};

}  // namespace Interpreter

#endif  // INTERPRETER_METHOD_CALL_EXPRESSION_NODE_HPP
