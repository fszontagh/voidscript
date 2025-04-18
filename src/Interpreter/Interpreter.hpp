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
    void runOperation(const Operations::Operation & op) {
        std::cout << "Operation: " << op.toString() << "\n";

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

            case Operations::Type::FunctionCall: {
                // Check that the called function is defined in the symbol table
                if (!Symbols::SymbolContainer::instance()->exists(op.targetName)) {
                    throw std::runtime_error("Function not declared: " + op.targetName);
                }
                break;
            }
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

  public:
    Interpreter() {}

    void run() {
        for (const auto & operation : Operations::Container::instance()->getAll()) {
            runOperation(*operation);
        }
    }

};  // class Interpreter

}  // namespace Interpreter
#endif  // INTERPRETER_HPP
