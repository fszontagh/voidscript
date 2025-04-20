#ifndef INTERPRETER_CALL_EXPRESSION_NODE_HPP
#define INTERPRETER_CALL_EXPRESSION_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/ReturnException.hpp"
#include "Modules/ModuleManager.hpp"
#include "Symbols/FunctionSymbol.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

/**
  * @brief Expression node representing a function call returning a value.
  */
class CallExpressionNode : public ExpressionNode {
    std::string                                  functionName_;
    std::vector<std::unique_ptr<ExpressionNode>> args_;
    // Source location for error reporting
    std::string                                  filename_;
    int                                          line_;
    size_t                                       column_;

  public:
    CallExpressionNode(std::string functionName, std::vector<std::unique_ptr<ExpressionNode>> args,
                       const std::string & filename, int line, size_t column) :
        functionName_(std::move(functionName)),
        args_(std::move(args)),
        filename_(filename),
        line_(line),
        column_(column) {}

    Symbols::Value evaluate(Interpreter & interpreter) const override {
        using namespace Symbols;
        try {
            // Evaluate argument expressions
            std::vector<Value> argValues;
            argValues.reserve(args_.size());
            for (const auto & expr : args_) {
                argValues.push_back(expr->evaluate(interpreter));
            }

            // Built-in function
            auto & mgr = Modules::ModuleManager::instance();
            if (mgr.hasFunction(functionName_)) {
                return mgr.callFunction(functionName_, argValues);
            }

            // User-defined function: lookup through scope hierarchy
            SymbolContainer *               sc       = SymbolContainer::instance();
            std::string                     lookupNs = sc->currentScopeName();
            std::shared_ptr<FunctionSymbol> funcSym;
            // Search for function symbol in current and parent scopes
            while (true) {
                std::string fnSymNs = lookupNs + "::functions";
                auto        sym     = sc->get(fnSymNs, functionName_);
                if (sym && sym->getKind() == Kind::Function) {
                    funcSym = std::static_pointer_cast<FunctionSymbol>(sym);
                    break;
                }
                auto pos = lookupNs.rfind("::");
                if (pos == std::string::npos) {
                    break;
                }
                lookupNs = lookupNs.substr(0, pos);
            }
            if (!funcSym) {
                throw std::runtime_error("Function not found: " + functionName_);
            }
            const auto & params = funcSym->parameters();
            if (params.size() != argValues.size()) {
                throw std::runtime_error("Function '" + functionName_ + "' expects " + std::to_string(params.size()) +
                                         " args, got " + std::to_string(argValues.size()));
            }
            // Enter function scope and bind parameters
            const std::string fnOpNs = funcSym->context() + "::" + functionName_;
            sc->enter(fnOpNs);
            for (size_t i = 0; i < params.size(); ++i) {
                const auto &  p      = params[i];
                const Value & v      = argValues[i];
                auto          varSym = SymbolFactory::createVariable(p.name, v, fnOpNs);
                sc->add(varSym);
            }
            // Execute function body operations and capture return
            Symbols::Value returnValue;
            auto           ops = Operations::Container::instance()->getAll(fnOpNs);
            for (const auto & op : ops) {
                try {
                    interpreter.runOperation(*op);
                } catch (const ReturnException & ret) {
                    returnValue = ret.value();
                    break;
                }
            }
            sc->enterPreviousScope();
            return returnValue;
        } catch (const std::exception & e) {
            throw ::Interpreter::Exception(e.what(), filename_, line_, column_);
        }
        // Unreachable: all paths either return or throw
        return Symbols::Value();
    }

    std::string toString() const override {
        return "CallExpressionNode{ function='" + functionName_ + "', args=" + std::to_string(args_.size()) + " }";
    }
};

}  // namespace Interpreter

#endif  // INTERPRETER_CALL_EXPRESSION_NODE_HPP
