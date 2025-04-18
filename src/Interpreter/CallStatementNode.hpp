#ifndef INTERPRETER_CALL_STATEMENT_NODE_HPP
#define INTERPRETER_CALL_STATEMENT_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
// Include for unified runtime Exception (inherits BaseException)
#include "BaseException.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "StatementNode.hpp"
#include "Symbols/FunctionSymbol.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"
#include "Symbols/Value.hpp"
#include "Modules/ModuleManager.hpp"

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
        using namespace Symbols;
        // Evaluate argument expressions
        std::vector<Value> argValues;
        argValues.reserve(args_.size());
        for (const auto & expr : args_) {
            argValues.push_back(expr->evaluate(interpreter));
        }

        // Handle built-in function callbacks
        {
            auto &mgr = Modules::ModuleManager::instance();
            if (mgr.hasFunction(functionName_)) {
                mgr.callFunction(functionName_, argValues);
                return;
            }
        }
        // Lookup function symbol in functions namespace
        SymbolContainer * sc        = SymbolContainer::instance();
        const std::string currentNs = sc->currentScopeName();
        const std::string fnSymNs   = currentNs + ".functions";
        auto              sym       = sc->get(fnSymNs, functionName_);
        if (!sym || sym->getKind() != Kind::Function) {
            throw Exception("Function not found: " + functionName_, filename_, line_, column_);
        }
        auto funcSym = std::static_pointer_cast<FunctionSymbol>(sym);

        // Check parameter count
        const auto & params = funcSym->parameters();
        if (params.size() != argValues.size()) {
            throw Exception(
                "Function '" + functionName_ + "' expects " + std::to_string(params.size()) +
                " args, got " + std::to_string(argValues.size()),
                filename_, line_, column_);
        }

        // Enter function scope to bind parameters and execute body
        const std::string fnOpNs = currentNs + "." + functionName_;
        sc->enter(fnOpNs);
        // Bind parameters as local variables
        for (size_t i = 0; i < params.size(); ++i) {
            const auto &  p = params[i];
            const Value & v = argValues[i];

            auto varSym = SymbolFactory::createVariable(p.name, v, fnOpNs);
            sc->add(varSym);
        }
        // Execute function body operations
        auto ops = Operations::Container::instance()->getAll(fnOpNs);
        auto it  = ops.begin();
        for (; it != ops.end(); ++it) {
            interpreter.runOperation(*(*it));
        }
        // Exit function scope
        sc->enterPreviousScope();
    }

    std::string toString() const override {
        return "CallStatementNode{ functionName='" + functionName_ + "', " +
               "args=" + std::to_string(args_.size()) + " " + "filename='" + filename_ + "', " +
               "line=" + std::to_string(line_) + ", " + "column=" + std::to_string(column_) + "}";
    };
};

}  // namespace Interpreter

// namespace Interpreter
#endif  // INTERPRETER_CALL_STATEMENT_NODE_HPP
