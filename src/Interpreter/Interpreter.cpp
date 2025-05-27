#include "Interpreter/Interpreter.hpp"

#include <iostream>

#include "Interpreter/ReturnException.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

void Interpreter::setThisObject(const Symbols::ValuePtr & obj) {
    thisObject_ = obj;
}

void Interpreter::clearThisObject() {
    thisObject_ = Symbols::ValuePtr();
}

const Symbols::ValuePtr & Interpreter::getThisObject() const {
    return thisObject_;
}

void Interpreter::run() {
    // Determine namespace to execute
    const std::string ns = Symbols::SymbolContainer::instance()->currentScopeName();
    for (const auto & operation : Operations::Container::instance()->getAll(ns)) {
        runOperation(*operation);
    }
}

void Interpreter::runOperation(const Operations::Operation & op) {
    if (debug_) {
        std::cerr << "[Debug][Interpreter] Operation: " << op.toString() << "\n";
    }

    if (!op.statement && op.type != Operations::Type::Error) {
        throw Exception("Invalid operation: missing statement", "-", 0, 0);
    }

    try {
        switch (op.type) {
            // Basic statements
            case Operations::Type::Declaration:
            case Operations::Type::Assignment:
            case Operations::Type::Expression:
                op.statement->interpret(*this);
                break;

            // Function-related operations
            case Operations::Type::FuncDeclaration:
            case Operations::Type::MethodDeclaration:
            case Operations::Type::FunctionCall:
            case Operations::Type::MethodCall:
            case Operations::Type::Return:
                op.statement->interpret(*this);
                break;

            // Control flow
            case Operations::Type::Conditional:
            case Operations::Type::Loop:
            case Operations::Type::While:
                op.statement->interpret(*this);
                break;

            // Flow control statements
            case Operations::Type::Break:
            case Operations::Type::Continue:
                throw Exception("Break/Continue not implemented", "-", 0, 0);

            // Module system
            case Operations::Type::Import:
                throw Exception("Import not implemented", "-", 0, 0);

            // Special cases
            case Operations::Type::Block:
                op.statement->interpret(*this);
                break;

            case Operations::Type::Error:
                throw Exception("Error operation encountered", "-", 0, 0);

            default:
                throw Exception("Unknown operation type", "-", 0, 0);
        }
    } catch (const Exception &) {
        throw;
    } catch (const std::exception & e) {
        throw Exception(e.what(), "-", 0, 0);
    }
}

unsigned long long Interpreter::get_unique_call_id() {
    return next_call_id_++;
}

}  // namespace Interpreter
