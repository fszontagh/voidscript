#ifndef SCRIPT_EXCEPTION_MACROS_H
#define SCRIPT_EXCEPTION_MACROS_H

#include "ScriptException.hpp"

//
// Purpose of macros: unified exception handling with extended error information (source file and line number)
//

// Invalid token type - expected different type
#define THROW_UNEXPECTED_TOKEN_ERROR(token, expected) \
    throw ScriptException::makeUnexpectedTokenError(token, expected, __FILE__, __LINE__)

#define THROW_UNEXPECTED_TOKEN_ERROR_HELPER(token, expected, file, line) \
    throw ScriptException::makeUnexpectedTokenError(token, expected, file, line)

// Accessing unknown (undefined) variable
#define THROW_UNDEFINED_VARIABLE_ERROR(name, token) \
    throw ScriptException::makeUndefinedVariableError(name, token, __FILE__, __LINE__)

#define THROW_UNDEFINED_VARIABLE_ERROR_HELPER(name, token, file, line) \
    throw ScriptException::makeUndefinedVariableError(name, token, file, line)

// Unknown (undefined) function call
#define THROW_UNDEFINED_FUNCTION_ERROR(name, token) \
    throw ScriptException::makeUndefinedFunctionError(name, token, __FILE__, __LINE__)

// Variable type mismatch - e.g. string instead of number
#define THROW_VARIABLE_TYPE_MISSMATCH_ERROR(target_variable_name, target_variable_type, source_variable_name,         \
                                            source_variable_type, token)                                              \
    throw ScriptException::makeVariableTypeMismatchError(target_variable_name, target_variable_type,                  \
                                                         source_variable_name, source_variable_type, token, __FILE__, \
                                                         __LINE__)

// Redefining a variable with the same name is not allowed
#define THROW_VARIABLE_REDEFINITION_ERROR(name, token) \
    throw ScriptException::makeVariableRedefinitionError(name, token, __FILE__, __LINE__)

#define THROW_FUNCTION_REDEFINITION_ERROR(name, token) \
    throw ScriptException::makeFunctionRedefinitionError(name, token, __FILE__, __LINE__)

// Invalid or incorrect function argument
#define THROW_INVALID_FUNCTION_ARGUMENT_ERROR(functionName, argName, token) \
    throw ScriptException::makeFunctionInvalidArgumentError(functionName, argName, token, __FILE__, __LINE__)

#endif  // SCRIPT_EXCEPTION_MACROS_H
