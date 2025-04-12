#ifndef SCRIPT_EXCEPTION_MACROS_H
#define SCRIPT_EXCEPTION_MACROS_H

#define THROW_UNEXPECTED_TOKEN_ERROR(token, expected) \
    SScriptInterpreter::throwUnexpectedTokenError(token, expected, __FILE__, __LINE__)

#define THROW_UNDEFINED_VARIABLE_ERROR(name, token) \
    SScriptInterpreter::throwUndefinedVariableError(name, token, __FILE__, __LINE__)

#define THROW_VARIABLE_TYPE_MISSMATCH_ERROR(target_variable_name, target_variable_type, source_variable_name,       \
                                            source_variable_type, token)                                            \
    SScriptInterpreter::throwVariableTypeMissmatchError(target_variable_name, target_variable_type,                  \
                                                       source_variable_name, source_variable_type, token, __FILE__, \
                                                       __LINE__)

#define THROW_VARIABLE_REDEFINITION_ERROR(name, token) \
    SScriptInterpreter::throwVariableRedefinitionError(name, token, __FILE__, __LINE__)

#endif  // SCRIPT_EXCEPTION_MACROS_H
