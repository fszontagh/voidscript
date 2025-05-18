#ifndef INTERPRETER_CALL_EXPRESSION_NODE_HPP
#define INTERPRETER_CALL_EXPRESSION_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/ReturnException.hpp"
#include "Modules/UnifiedModuleManager.hpp"
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

    Symbols::Value::ValuePtr evaluate(Interpreter & interpreter) const override {
        using namespace Symbols;
        try {
            // Evaluate argument expressions
            std::vector<Symbols::Value::ValuePtr> argValues;
            argValues.reserve(args_.size());
            for (const auto & expr : args_) {
                argValues.push_back(expr->evaluate(interpreter));
            }

            // Built-in function
            auto & mgr = Modules::UnifiedModuleManager::instance();
            if (mgr.hasFunction(functionName_)) {
                return mgr.callFunction(functionName_, argValues);
            }

            // User-defined function: lookup through scope hierarchy
            SymbolContainer *               sc       = SymbolContainer::instance();
            std::string                     lookupNs = sc->currentScopeName();
            std::shared_ptr<FunctionSymbol> funcSym;
            // Search for function symbol in current and parent scopes
            while (true) {
                auto               scope_table = sc->getScopeTable(lookupNs);
                Symbols::SymbolPtr sym         = nullptr;
                if (scope_table) {
                    sym = scope_table->get(Symbols::SymbolContainer::DEFAULT_FUNCTIONS_SCOPE, functionName_);
                }

                if (sym && sym->getKind() == Kind::Function) {
                    funcSym = std::static_pointer_cast<FunctionSymbol>(sym);
                    break;
                }
                // Move to parent scope by finding the last '::'
                // This assumes scope names are hierarchical like /file/path::class::method or a top-level file scope
                auto pos = lookupNs.rfind(Symbols::SymbolContainer::SCOPE_SEPARATOR);
                if (pos == std::string::npos) {
                    // If lookupNs doesn't contain "::", it means we've reached the top-level scope (e.g., a file path).
                    // If the function wasn't found by now, it's not in any accessible parent scope.
                    break;
                }
                lookupNs = lookupNs.substr(0, pos);
                // If substr results in an empty string (e.g. if lookupNs was "::foo"),
                // it implies an invalid scope name, so break.
                if (lookupNs.empty()) {
                    break;
                }
            }
            if (!funcSym) {
                throw std::runtime_error("Function not found: " + functionName_);
            }
            const auto & params  = funcSym->parameters();
            const auto   retType = funcSym->returnType();

            if (params.size() != argValues.size()) {
                throw std::runtime_error("Function '" + functionName_ + "' expects " + std::to_string(params.size()) +
                                         " args, got " + std::to_string(argValues.size()));
            }

            // Create unique scope for this function call
            const std::string canonical_fn_scope_name =
                funcSym->context().empty() ?
                    functionName_ :
                    funcSym->context() + Symbols::SymbolContainer::SCOPE_SEPARATOR + functionName_;
            std::string unique_call_scope_name = canonical_fn_scope_name + Symbols::SymbolContainer::CALL_SCOPE +
                                                 std::to_string(Interpreter::get_unique_call_id());

            sc->create(unique_call_scope_name);  // Creates and enters the new unique scope

            // Bind parameters in the unique call scope
            for (size_t i = 0; i < params.size(); ++i) {
                const auto & p      = params[i];
                auto &       v      = argValues[i];
                // Symbol's context is this specific call's scope
                auto         varSym = Symbols::SymbolFactory::createVariable(p.name, v, unique_call_scope_name);
                sc->add(varSym);  // Adds to the current scope (unique_call_scope_name)
            }

            // Execute function body operations. These operations will use the current unique_call_scope_name.
            Symbols::Value::ValuePtr returnValue;
            // Operations are associated with the canonical function name (where they are defined/parsed)
            auto                     ops = Operations::Container::instance()->getAll(canonical_fn_scope_name);
            for (const auto & op : ops) {
                try {
                    interpreter.runOperation(*op);
                } catch (const ReturnException & ret) {
                    returnValue = ret.value();
                    break;
                }
            }
            sc->enterPreviousScope();  // Exit unique_call_scope_name
            if (returnValue->getType() != retType) {
                throw std::runtime_error("Function " + functionName_ + " expected return type is " +
                                         Symbols::Variables::TypeToString(retType) + " got " +
                                         Symbols::Variables::TypeToString(returnValue->getType()));
            }
            return returnValue;
        } catch (const std::exception & e) {
            throw ::Interpreter::Exception(e.what(), filename_, line_, column_);
        }
        // Unreachable: all paths either return or throw
        return nullptr;
    }

    std::string toString() const override {
        return "CallExpressionNode{ function='" + functionName_ + "', args=" + std::to_string(args_.size()) + " }";
    }
};

}  // namespace Interpreter

#endif  // INTERPRETER_CALL_EXPRESSION_NODE_HPP
