#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP
#include <iostream>
#include <memory>
#include <stdexcept>

#include "Interpreter/Operation.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Symbols/SymbolContainer.hpp"

namespace Interpreter {

class Interpreter {
  private:
    bool debug_ = false;
  public:
    /**
     * @brief Construct interpreter with optional debug output
     * @param debug enable interpreter debug output
     */
    Interpreter(bool debug = false) : debug_(debug) {}

    /**
     * @brief Execute all operations in the current namespace (e.g., file-level or function-level).
     */
    void run() {
        // Determine namespace to execute
        const std::string ns = Symbols::SymbolContainer::instance()->currentScopeName();
        for (const auto & operation : Operations::Container::instance()->getAll(ns)) {
            runOperation(*operation);
        }
    }

    void runOperation(const Operations::Operation & op) {
        if (debug_) {
            std::cerr << "[Debug][Interpreter] Operation: " << op.toString() << "\n";
        }

        switch (op.type) {
            case Operations::Type::Declaration:
                if (op.statement) {
                    op.statement->interpret(*this);
                }
                break;
            case Operations::Type::FuncDeclaration:
                {
                    op.statement->interpret(*this);
                }
                break;

            case Operations::Type::Assignment:
                {
                    op.statement->interpret(*this);
                    break;
                }

            case Operations::Type::Expression:
                {
                    op.statement->interpret(*this);  // csak side effect miatt
                    break;
                }

            case Operations::Type::FunctionCall:
                if (op.statement) {
                    op.statement->interpret(*this);
                }
                break;
            case Operations::Type::Return:
            case Operations::Type::Loop:
            case Operations::Type::Break:
            case Operations::Type::Continue:
            case Operations::Type::Block:
            case Operations::Type::Import:
            case Operations::Type::Error:
            case Operations::Type::Conditional:
                // TODO: implement these operations later
                break;
            default:
                throw std::runtime_error("Not implemented operation type");
        }
    }

};  // class Interpreter

}  // namespace Interpreter
#endif  // INTERPRETER_HPP
