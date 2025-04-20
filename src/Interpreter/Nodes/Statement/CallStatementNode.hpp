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
                std::string fnSymNs = lookupNs + ".functions";
                auto        sym     = sc->get(fnSymNs, functionName_);
                if (sym && sym->getKind() == Kind::Function) {
                    funcSym = std::static_pointer_cast<FunctionSymbol>(sym);
                    break;
                }
                auto pos = lookupNs.find_last_of('.');
                if (pos == std::string::npos) {
                    break;
                }
                lookupNs = lookupNs.substr(0, pos);
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
            // Enter function scope and bind parameters
            const std::string fnOpNs = funcSym->context() + "." + functionName_;
            sc->enter(fnOpNs);
            for (size_t i = 0; i < params.size(); ++i) {
                const auto &  p      = params[i];
                const Value & v      = argValues[i];
                auto          varSym = SymbolFactory::createVariable(p.name, v, fnOpNs);
                sc->add(varSym);
            }
            auto ops = Operations::Container::instance()->getAll(fnOpNs);
            for (const auto & op : ops) {
                interpreter.runOperation(*op);
            }
            sc->enterPreviousScope();
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
