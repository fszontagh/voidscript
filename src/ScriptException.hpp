#ifndef SCRIPTEXCEPTION_HPP
#define SCRIPTEXCEPTION_HPP

#include <stdexcept>
#include <string>

#include "options.h"
#include "Token.hpp"

enum class ScriptErrorType : std::uint8_t {
    UnexpectedToken,
    UndefinedVariable,
    UndefinedFunction,
    VariableTypeMismatch,
    VariableRedefinition,
    Custom
};

class ScriptException : public std::runtime_error {
  public:
    ScriptException(ScriptErrorType type, const std::string & message, const std::string & file = "", int line = 0,
                    const Token & token = Token()) :
        std::runtime_error(message),
        type_(type),
        file_(file),
        line_(line),
        token_(token),
        fullMessage_(formatMessage(message)) {}

    const char * what() const noexcept override { return fullMessage_.c_str(); }

    ScriptErrorType type() const { return type_; }

    const std::string & file() const { return file_; }

    int line() const { return line_; }

    const Token & token() const { return token_; }

    static ScriptException makeUnexpectedEndOfFileError(const Token & token, const std::string & file = "",
                                                        int line = 0) {
        std::string msg = "unexpected end of file";
        if (!token.lexeme.empty()) {
            msg += " near '" + token.lexeme + "'";
        }
        return ScriptException(ScriptErrorType::UnexpectedToken, msg, file, line, token);
    }

    static ScriptException makeUnexpectedTokenError(const Token & token, const std::string & expected = "",
                                                    const std::string & file = "", int line = 0) {
        std::string msg = "unexpected token: '" + token.lexeme + "'";

#if DEBUG_BUILD == 1
        msg.append(" token type: " + tokenTypeNames.at(token.type));
#endif

        if (!expected.empty()) {
            msg += ", expected " + expected;
        }
        return ScriptException(ScriptErrorType::UnexpectedToken, msg, file, line, token);
    }

    static ScriptException makeUndefinedVariableError(const std::string & name, const Token & token,
                                                      const std::string & file = "", int line = 0) {
        std::string msg = "undefined variable: '$" + name + "'";
        return ScriptException(ScriptErrorType::UndefinedVariable, msg, file, line, token);
    }

    static ScriptException makeUndefinedFunctionError(const std::string & name, const Token & token,
                                                      const std::string & file = "", int line = 0) {
        std::string msg = "undefined function: '" + name + "'";
#if DEBUG_BUILD == 1
        msg.append(", type: " + getTokenTypeAsString(token.type));
#endif
        return ScriptException(ScriptErrorType::UndefinedFunction, msg, file, line, token);
    }

    static ScriptException makeVariableRedefinitionError(const std::string & name, const Token & token,
                                                         const std::string & file = "", int line = 0) {
        std::string msg = "variable already defined: '" + name + "'";
        return ScriptException(ScriptErrorType::VariableRedefinition, msg, file, line, token);
    }

    static ScriptException makeVariableTypeMismatchError(const std::string & targetVar, const std::string & targetType,
                                                         const std::string & sourceVar, const std::string & sourceType,
                                                         const Token & token, const std::string & file = "",
                                                         int line = 0) {
        std::string msg = "variable type mismatch: '$" + targetVar + "' declared type: '" + targetType + "'";
        if (!sourceVar.empty()) {
            msg += ", source variable: '" + sourceVar + "'";
        }
        if (!sourceType.empty()) {
            msg += ", assigned type: '" + sourceType + "'";
        }
        return ScriptException(ScriptErrorType::VariableTypeMismatch, msg, file, line, token);
    }

    static ScriptException makeFunctionRedefinitionError(const std::string & name, const Token & token,
                                                         const std::string & file = "", int line = 0) {
        std::string msg = "variable already defined: '" + name + "'";
        return ScriptException(ScriptErrorType::VariableRedefinition, msg, file, line, token);
    }

    static ScriptException makeFunctionInvalidArgumentError(const std::string & functionName,
                                                            const std::string & argName, const Token & token,
                                                            const std::string & file = "", int line = 0) {
        std::string msg = "invalid argument for function '" + functionName + "': '" + argName + "'";
        return ScriptException(ScriptErrorType::Custom, msg, file, line, token);
    }

    static ScriptException makeFunctionArgumentCountMismatchError(const std::string & functionName,
                                                                  const size_t & expected, size_t actual,
                                                                  const Token & token, const std::string & file = "",
                                                                  int line = 0) {
        std::string msg = "invalid argument count for function '" + functionName + "', expected " +
                          std::to_string(expected) + ", got " + std::to_string(actual);
        return ScriptException(ScriptErrorType::Custom, msg, file, line, token);
    }

    static ScriptException makeFunctionBodyEmptyError(const std::string & functionName, const Token & token,
                                                      const std::string & file = "", int line = 0) {
        std::string msg = "function '" + functionName + "' has no body";
        return ScriptException(ScriptErrorType::Custom, msg, file, line, token);
    }

  private:
    ScriptErrorType type_;
    std::string     file_;
    int             line_;
    Token           token_;
    std::string     fullMessage_;

    std::string formatMessage(const std::string & base) const {
        std::string formatted = base;
        if (!token_.file.empty()) {
            formatted += " in file: " + token_.file + ":" + std::to_string(token_.lineNumber) + ":" +
                         std::to_string(token_.columnNumber);
        }
#if DEBUG_BUILD == 1
        if (!file_.empty()) {
            formatted = file_ + ":" + std::to_string(line_) + "\n" + formatted;
        }
#endif
        return formatted;
    }
};

#endif  // SCRIPTEXCEPTION_HPP
