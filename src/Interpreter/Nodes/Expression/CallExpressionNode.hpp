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
            auto & mgr = Modules::UnifiedModuleManager::instance();
            if (mgr.hasFunction(functionName_)) {
                return mgr.callFunction(functionName_, argValues);
            }

            // User-defined function: lookup through scope hierarchy
            SymbolContainer* sc = SymbolContainer::instance();
            
            // Debug current scope
            interpreter.debugLog("Looking for function '" + functionName_ + "' starting from scope: " + sc->currentScopeName());
            
            // First try direct function lookup in functions namespace
            std::shared_ptr<FunctionSymbol> funcSym;
            
            // Try to find the function in any accessible scope
            for (auto scopeName : sc->getScopeNames()) {
                auto scopeTable = sc->getScopeTable(scopeName);
                if (scopeTable) {
                    // Check in the functions namespace first
                    auto sym = scopeTable->get(SymbolContainer::DEFAULT_FUNCTIONS_SCOPE, functionName_);
                    if (sym && sym->getKind() == Kind::Function) {
                        funcSym = std::static_pointer_cast<FunctionSymbol>(sym);
                        interpreter.debugLog("Found function '" + functionName_ + "' in scope: " + scopeName + 
                                            " (namespace: " + SymbolContainer::DEFAULT_FUNCTIONS_SCOPE + ")");
                        break;
                    }
                    
                    // If not found in functions namespace, try other standard namespaces
                    const std::vector<std::string> namespaces = {
                        SymbolContainer::DEFAULT_VARIABLES_SCOPE,
                        SymbolContainer::DEFAULT_CONSTANTS_SCOPE,
                        SymbolContainer::DEFAULT_OTHERS_SCOPE
                    };
                    
                    for (const auto& ns : namespaces) {
                        sym = scopeTable->get(ns, functionName_);
                        if (sym && sym->getKind() == Kind::Function) {
                            funcSym = std::static_pointer_cast<FunctionSymbol>(sym);
                            interpreter.debugLog("Found function '" + functionName_ + "' in scope: " + scopeName + 
                                                " (namespace: " + ns + ")");
                            break;
                        }
                    }
                    
                    if (funcSym) break; // Exit outer loop if function found
                }
            }
            
            if (!funcSym) {
                // Debug output of all available functions in all scopes
                interpreter.debugLog("Function not found: " + functionName_ + ". Available functions:");
                for (auto scopeName : sc->getScopeNames()) {
                    auto scopeTable = sc->getScopeTable(scopeName);
                    if (scopeTable) {
                        // Check for functions in all standard namespaces
                        const std::vector<std::string> namespaces = {
                            SymbolContainer::DEFAULT_FUNCTIONS_SCOPE,
                            SymbolContainer::DEFAULT_VARIABLES_SCOPE,
                            SymbolContainer::DEFAULT_CONSTANTS_SCOPE,
                            SymbolContainer::DEFAULT_OTHERS_SCOPE
                        };
                        
                        for (const auto& ns : namespaces) {
                            auto symbols = scopeTable->listAll(ns);
                            for (auto sym : symbols) {
                                if (sym->getKind() == Kind::Function) {
                                    interpreter.debugLog("  - " + sym->name() + " in scope: " + scopeName + 
                                                        " (namespace: " + ns + ")");
                                }
                            }
                        }
                    }
                }
                throw std::runtime_error("Function not found: " + functionName_);
            }
            
            const auto & params = funcSym->parameters();
            if (params.size() != argValues.size()) {
                throw std::runtime_error("Function '" + functionName_ + "' expects " + std::to_string(params.size()) +
                                         " args, got " + std::to_string(argValues.size()));
            }

            // Create unique scope for this function call
            const std::string canonical_fn_scope_name =
                funcSym->context().empty() ? functionName_ : funcSym->context() + "::" + functionName_;
            std::string unique_call_scope_name =
                canonical_fn_scope_name + "::call_" + std::to_string(interpreter.getUniqueCallId());

            sc->create(unique_call_scope_name);  // Creates and enters the new unique scope

            // Bind parameters in the unique call scope
            for (size_t i = 0; i < params.size(); ++i) {
                const auto &  p      = params[i];
                const Value & v      = argValues[i];
                // Symbol's context is this specific call's scope
                auto          varSym = SymbolFactory::createVariable(p.name, v, unique_call_scope_name);
                sc->add(varSym);  // Adds to the current scope (unique_call_scope_name)
            }

            // Execute function body operations. These operations will use the current unique_call_scope_name.
            Symbols::Value returnValue;
            // Operations are associated with the canonical function name (where they are defined/parsed)
            auto           ops = Operations::Container::instance()->getAll(canonical_fn_scope_name);
            for (const auto & op : ops) {
                try {
                    interpreter.runOperation(*op);
                } catch (const ReturnException & ret) {
                    returnValue = ret.value();
                    break;
                }
            }
            sc->enterPreviousScope();  // Exit unique_call_scope_name
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

