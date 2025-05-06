#ifndef INTERPRETER_CALL_STATEMENT_NODE_HPP
#define INTERPRETER_CALL_STATEMENT_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Modules/ModuleManager.hpp"
#include "Symbols/FunctionSymbol.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

/**
 * @brief Statement node representing a function call with argument expressions.
 */
class CallStatementNode : public StatementNode {
    std::string                                  functionName_;
    std::vector<std::unique_ptr<ExpressionNode>> args_;

  public:
    CallStatementNode(const std::string & functionName, std::vector<std::unique_ptr<ExpressionNode>> args,
                      const std::string & file_name, int file_line, size_t column) :
        StatementNode(file_name, file_line, column),
        functionName_(functionName),
        args_(std::move(args)) {}

    void interpret(Interpreter & interpreter) const override {
        try {
            using namespace Symbols;
            std::vector<Value> argValues;
            argValues.reserve(args_.size());
            for (const auto & expr : args_) {
                argValues.push_back(expr->evaluate(interpreter));
            }
            {
                auto & mgr = Modules::ModuleManager::instance();
                if (mgr.hasFunction(functionName_)) {
                    mgr.callFunction(functionName_, argValues);
                    return;
                }
            }
            // User-defined function: lookup through scope hierarchy
            SymbolContainer *               sc       = SymbolContainer::instance();
            std::string                     lookupNs = sc->currentScopeName();
            std::shared_ptr<FunctionSymbol> funcSym;
            // Search for function symbol in current and parent scopes
            while (true) {
                auto scope_table = sc->getScopeTable(lookupNs);
                Symbols::SymbolPtr sym = nullptr;
                if (scope_table) {
                    sym = scope_table->get(Symbols::SymbolContainer::DEFAULT_FUNCTIONS_SCOPE, functionName_);
                }

                if (sym && sym->getKind() == Kind::Function) {
                    funcSym = std::static_pointer_cast<FunctionSymbol>(sym);
                    break;
                }
                // Move to parent scope by finding the last '::'
                // This assumes scope names are hierarchical (e.g., /file/path::class::method or a top-level file path)
                auto pos = lookupNs.rfind("::");
                if (pos == std::string::npos) {
                    // Reached top-level scope (e.g., a file path). If not found, it's not in accessible parent scopes.
                    break; 
                }
                lookupNs = lookupNs.substr(0, pos);
                // If substr results in an empty string (e.g. if lookupNs was "::foo"), implies invalid scope.
                if (lookupNs.empty()) break;
            }
            if (!funcSym) {
                throw Exception("Function not found: " + functionName_, filename_, line_, column_);
            }
            const auto & params = funcSym->parameters();
            if (params.size() != argValues.size()) {
                throw Exception("Function '" + functionName_ + "' expects " + std::to_string(params.size()) +
                                    " args, got " + std::to_string(argValues.size()),
                                filename_, line_, column_);
            }

            // Create unique scope for this function call
            const std::string canonical_fn_scope_name = funcSym->context().empty() ? functionName_ : funcSym->context() + "::" + functionName_;
            std::string unique_call_scope_name = canonical_fn_scope_name + "::call_" + std::to_string(Interpreter::get_unique_call_id());

            sc->create(unique_call_scope_name); // Creates and enters the new unique scope

            // Bind parameters in the unique call scope
            for (size_t i = 0; i < params.size(); ++i) {
                const auto &  p      = params[i];
                const Value & v      = argValues[i];
                // Symbol's context is this specific call's scope
                auto varSym = SymbolFactory::createVariable(p.name, v, unique_call_scope_name);
                sc->add(varSym); // Adds to the current scope (unique_call_scope_name)
            }

            // Operations are associated with the canonical function name
            auto ops = Operations::Container::instance()->getAll(canonical_fn_scope_name);
            for (const auto & op : ops) {
                interpreter.runOperation(*op); // These operations will use the current unique_call_scope_name
            }
            sc->enterPreviousScope(); // Exit unique_call_scope_name
        } catch (const Exception &) {
            throw;
        } catch (const std::exception & e) {
            throw Exception(e.what(), filename_, line_, column_);
        }
    }

    std::string toString() const override {
        return "CallStatementNode{ functionName='" + functionName_ + "', " + "args=" + std::to_string(args_.size()) +
               " " + "filename='" + filename_ + "', " + "line=" + std::to_string(line_) + ", " +
               "column=" + std::to_string(column_) + "}";
    };
};

}  // namespace Interpreter

// namespace Interpreter
#endif  // INTERPRETER_CALL_STATEMENT_NODE_HPP
