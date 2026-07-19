#ifndef INTERPRETER_THROW_EXCEPTION_HPP
#define INTERPRETER_THROW_EXCEPTION_HPP

#include <string>
#include <utility>

#include "BaseException.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

/**
 * @brief Raised by a script-level `throw expr;` and caught by `try`/`catch`.
 *
 * Derives from BaseException so that an uncaught throw is reported by the same
 * top-level handler as any other error rather than aborting the process.
 */
class ThrowException : public BaseException {
  public:
    explicit ThrowException(Symbols::ValuePtr value, const std::string & filename, int line, size_t column) :
        value_(std::move(value)) {
        rawMessage_ = value_->toString();
        if (filename == "-") {
            context_ = "At line: " + std::to_string(line) + ", column: " + std::to_string(column);
        } else {
            context_ = std::string(" in file \"") + filename + "\" at line: " + std::to_string(line) +
                       ", column: " + std::to_string(column);
        }
        formattedMessage_ = formatMessage();
    }

    std::string formatMessage() const override {
        return std::string("[Uncaught throw] >>") + context_ + " << : " + rawMessage_;
    }

    const Symbols::ValuePtr & value() const { return value_; }

  private:
    Symbols::ValuePtr value_;
};

}  // namespace Interpreter

#endif  // INTERPRETER_THROW_EXCEPTION_HPP
