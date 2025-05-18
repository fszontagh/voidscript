#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include <iostream>
#include <memory>
#include <stdexcept>

#include "BaseException.hpp"
#include "Interpreter/Operation.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Symbols/SymbolContainer.hpp"

namespace Interpreter {
class Exception : public BaseException {
  public:
    Exception(const std::string & msg, const std::string & filename, int line, size_t column) {
        rawMessage_ = msg;
        if (filename == "-") {
            context_ = "At line: " + std::to_string(line) + ", column: " + std::to_string(column);
        } else {
            context_ = std::string(" in file \"") + filename + "\" at line: " + std::to_string(line) +
                       ", column: " + std::to_string(column);
        }
        formattedMessage_ = formatMessage();
    }

    std::string formatMessage() const override {
        return std::string("[Runtime ERROR] >>") + context_ + " << : " + rawMessage_;
    }
};
}  // namespace Interpreter

namespace Interpreter {

class Interpreter {
  private:
    bool debug_ = false;
    static inline unsigned long long next_call_id_ = 0;

  public:
    /**
     * @brief Construct interpreter with optional debug output
     * @param debug enable interpreter debug output
     */
    Interpreter(bool debug = false) : debug_(debug) {}

    static unsigned long long get_unique_call_id() {
        return next_call_id_++;
    }

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
            case Operations::Type::Assignment:
            case Operations::Type::Expression:
            case Operations::Type::FuncDeclaration:
                {
                    op.statement->interpret(*this);
                }
                break;

            case Operations::Type::FunctionCall:
            case Operations::Type::Return:
            case Operations::Type::Conditional:
                if (op.statement) {
                    op.statement->interpret(*this);
                    break;
                }
            case Operations::Type::Loop:
            case Operations::Type::While:
                // for-in or while loop
                if (op.statement) {
                    op.statement->interpret(*this);
                }
                break;
            case Operations::Type::Break:
            case Operations::Type::Continue:
            case Operations::Type::Block:
            case Operations::Type::Import:
            case Operations::Type::Error:
                // TODO: implement these operations later
                break;
            default:
                throw std::runtime_error("Not implemented operation type");
        }
    }
};  // class Interpreter

}  // namespace Interpreter
#endif  // INTERPRETER_HPP
