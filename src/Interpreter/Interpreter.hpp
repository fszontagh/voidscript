#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP
#include <iostream>
#include <memory>
#include <stdexcept>
#include <atomic>
#include <cstdint>
#include <string>

#include "BaseException.hpp"
#include "Interpreter/Operation.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/ReturnException.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

// Exception type for runtime errors, includes file, line, and column context
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

inline std::atomic<std::uint64_t> call_id_counter = 0;

inline std::uint64_t get_unique_call_id() { return call_id_counter.fetch_add(1, std::memory_order_relaxed); }

class Interpreter {
  private:
    bool                             debug_        = false;

  public:
    /**
     * @brief Construct interpreter with optional debug output
     * @param debug enable interpreter debug output
     */
    Interpreter(bool debug = false) : debug_(debug) {}

    /**
     * @brief Get a unique call ID for function calls
     * @return A unique integer ID
     */
    std::uint64_t getUniqueCallId() const {
        return get_unique_call_id();
    }

    /**
     * @brief Execute all operations in the current namespace (e.g., file-level or function-level).
     */
    void run() {
        // Determine namespace to execute
        const std::string ns = Symbols::SymbolContainer::instance()->currentScopeName();
        for (const auto & operation : Operations::Container::instance()->getAll(ns)) {
            try {
                runOperation(*operation);
            } catch (const ReturnException & re) {
                // Ignore return exceptions at top level
                debugLog("Caught return exception at top level, ignoring");
            } catch (const std::exception & e) {
                debugLog(std::string("Error executing operation: ") + e.what());
            }
        }
    }

    void runOperation(const Operations::Operation & op) {
        if (debug_) {
            std::cerr << "[Debug][Interpreter] Running operation: " << op.toString() << std::endl;
        }
        if (op.statement) {
            op.statement->interpret(*this);
        } else {
            throw std::runtime_error("Null statement in operation: " + op.targetName);
        }
    }

    /**
     * @brief Output a debug message if debug mode is enabled.
     * @param message The message to output.
     */
    void debugLog(const std::string & message) const {
        if (debug_) {
            std::cerr << "[Debug][Interpreter] " << message << std::endl;
        }
    }

};  // class Interpreter

}  // namespace Interpreter
#endif  // INTERPRETER_HPP
