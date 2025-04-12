#ifndef SCRIPT_EXCEPTION_MACROS_H
#define SCRIPT_EXCEPTION_MACROS_H

#define THROW_UNEXPECTED_TOKEN_ERROR(token, expected) \
    ScriptInterpreter::throwUnexpectedTokenError(token, expected, __FILE__, __LINE__)

#define THROW_UNDEFINED_VARIABLE_ERROR(name, token) \
    ScriptInterpreter::throwUndefinedVariableError(name, token, __FILE__, __LINE__)

#define THROW_VARIABLE_TYPE_MISSMATCH_ERROR(target_variable_name, target_variable_type, source_variable_name,       \
                                            source_variable_type, token)                                            \
    ScriptInterpreter::throwVariableTypeMissmatchError(target_variable_name, target_variable_type,                  \
                                                       source_variable_name, source_variable_type, token, __FILE__, \
                                                       __LINE__)

#define THROW_VARIABLE_REDEFINITION_ERROR(name, token) \
    ScriptInterpreter::throwVariableRedefinitionError(name, token, __FILE__, __LINE__)

#endif  // SCRIPT_EXCEPTION_MACROS_H
