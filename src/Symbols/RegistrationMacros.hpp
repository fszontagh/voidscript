#ifndef REGISTRATION_MACROS_HPP
#define REGISTRATION_MACROS_HPP

#include "Symbols/SymbolContainer.hpp"

/**
 * @brief Register a function with documentation
 * @param fnName Name of the function
 * @param retType Return type of the function
 * @param paramListVec Vector of parameter information
 * @param docStr Description of the function
 * @param callback Function implementation
 */
#define REGISTER_FUNCTION(fnName, retType, paramListVec, docStr, callback)         \
    do {                                                                            \
        Symbols::SymbolContainer::instance()->registerFunction(fnName, callback, retType); \
        Symbols::SymbolContainer::instance()->registerDoc(                          \
            fnName,                                                                 \
            Symbols::FunctionDoc{ fnName, retType, paramListVec, docStr });         \
    } while (0)

/**
 * @brief Register a class
 * @param className Name of the class
 */
#define REGISTER_CLASS(className)                                                   \
    do {                                                                            \
        if (!Symbols::SymbolContainer::instance()->hasClass(className)) {           \
            Symbols::SymbolContainer::instance()->registerClass(                    \
                className, Symbols::SymbolContainer::instance()->getCurrentModule()); \
        }                                                                           \
    } while (0)

/**
 * @brief Register a method with documentation
 * @param className Name of the class
 * @param methodName Name of the method
 * @param paramList Vector of parameter information
 * @param callback Method implementation
 * @param retType Return type of the method
 * @param docStr Description of the method
 */
#define REGISTER_METHOD(className, methodName, paramList, callback, retType, docStr) \
    do {                                                                             \
        std::string fullMethodName =                                                 \
            std::string(className) + Symbols::SymbolContainer::SCOPE_SEPARATOR + std::string(methodName); \
        Symbols::SymbolContainer::instance()->addNativeMethod(                       \
            className, methodName, callback, retType, paramList);                    \
        Symbols::SymbolContainer::instance()->registerDoc(                           \
            fullMethodName,                                                          \
            Symbols::FunctionDoc{ fullMethodName, retType, paramList, docStr });     \
    } while (0)

/**
 * @brief Register a property for a class
 * @param className Name of the class
 * @param propertyName Name of the property
 * @param type Type of the property
 * @param defaultValue Default value expression (optional)
 */
#define REGISTER_PROPERTY(className, propertyName, type, defaultValue) \
    Symbols::SymbolContainer::instance()->addProperty(className, propertyName, type, false, defaultValue)

#endif // REGISTRATION_MACROS_HPP