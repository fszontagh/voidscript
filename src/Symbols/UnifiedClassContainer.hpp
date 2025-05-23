#ifndef SYMBOL_UNIFIED_CLASS_CONTAINER_HPP
#define SYMBOL_UNIFIED_CLASS_CONTAINER_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <optional>

#include "Modules/BaseModule.hpp"
#include "Parser/ParsedExpression.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"
#include "BaseException.hpp"

namespace Symbols {

/**
 * @brief Container for class definitions in the SoniScript language.
 * 
 * This class manages class definitions, their properties, methods, and inheritance.
 * It provides methods for registering and querying class information.
 */
class UnifiedClassContainer {
public:
    /**
     * @brief Parameter information for class methods
     */
    struct ParameterInfo {
        std::string name;
        Variables::Type type;
        std::string description = "";
        bool optional = false;
        bool interpolate = false;
    };

    /**
     * @brief Property information for class properties
     */
    struct PropertyInfo {
        std::string name;
        Variables::Type type;
        Parser::ParsedExpressionPtr defaultValueExpr;
        bool isPrivate = false;
    };

    /**
     * @brief Method information for class methods
     */
    struct MethodInfo {
        std::string name;
        std::string qualifiedName;
        Variables::Type returnType;
        std::vector<ParameterInfo> parameters;
        bool isPrivate = false;
        std::function<ValuePtr(const std::vector<ValuePtr>&)> nativeImplementation;
    };

    /**
     * @brief Full class definition information
     */
    struct ClassInfo {
        std::string name;
        std::string parentClass; // For inheritance
        std::vector<PropertyInfo> properties;
        std::vector<MethodInfo> methods;
        std::unordered_map<std::string, ValuePtr> staticProperties; // For static properties
        Modules::BaseModule* module = nullptr; // Module that defined this class
    };

    /**
     * @brief Exception class for UnifiedClassContainer errors
     */
    class ClassException : public BaseException {
    public:
        explicit ClassException(const std::string& message) : BaseException(message) {}
    };

    /**
     * @brief Constructor
     */
    UnifiedClassContainer();

    /**
     * @brief Destructor
     */
    virtual ~UnifiedClassContainer();

    /**
     * @brief Register a new class
     * @param className Name of the class to register
     * @param module Pointer to the module that defines this class (nullptr for script-defined classes)
     * @return Reference to the newly created ClassInfo
     * @throws ClassException if the class is already registered
     */
    ClassInfo& registerClass(const std::string& className, Modules::BaseModule* module = nullptr);

    /**
     * @brief Register a new class with inheritance
     * @param className Name of the class to register
     * @param parentClassName Name of the parent class
     * @param module Pointer to the module that defines this class (nullptr for script-defined classes)
     * @return Reference to the newly created ClassInfo
     * @throws ClassException if the class is already registered or the parent class doesn't exist
     */
    ClassInfo& registerClass(const std::string& className, const std::string& parentClassName, 
                            Modules::BaseModule* module = nullptr);

    /**
     * @brief Check if a class is registered
     * @param className Name of the class to check
     * @return True if the class is registered, false otherwise
     */
    bool hasClass(const std::string& className) const;

    /**
     * @brief Get information about a registered class
     * @param className Name of the class to get information for
     * @return Reference to the ClassInfo for the class
     * @throws ClassException if the class is not registered
     */
    ClassInfo& getClassInfo(const std::string& className);

    /**
     * @brief Get information about a registered class (const version)
     * @param className Name of the class to get information for
     * @return Const reference to the ClassInfo for the class
     * @throws ClassException if the class is not registered
     */
    const ClassInfo& getClassInfo(const std::string& className) const;

    /**
     * @brief Add a property to a class
     * @param className Name of the class to add the property to
     * @param propertyName Name of the property
     * @param type Type of the property
     * @param isPrivate Whether the property is private
     * @param defaultValueExpr Expression for the default value (optional)
     * @throws ClassException if the class is not registered or the property already exists
     */
    void addProperty(const std::string& className, const std::string& propertyName, 
                    Variables::Type type, bool isPrivate = false, 
                    Parser::ParsedExpressionPtr defaultValueExpr = nullptr);

    /**
     * @brief Add a method to a class
     * @param className Name of the class to add the method to
     * @param methodName Name of the method
     * @param returnType Return type of the method
     * @param parameters Method parameters
     * @param isPrivate Whether the method is private
     * @throws ClassException if the class is not registered or the method already exists
     */
    void addMethod(const std::string& className, const std::string& methodName,
                  Variables::Type returnType = Variables::Type::NULL_TYPE,
                  const std::vector<ParameterInfo>& parameters = {},
                  bool isPrivate = false);

    /**
     * @brief Add a native method to a class
     * @param className Name of the class to add the method to
     * @param methodName Name of the method
     * @param implementation Native implementation of the method
     * @param returnType Return type of the method
     * @param parameters Method parameters
     * @param isPrivate Whether the method is private
     * @throws ClassException if the class is not registered or the method already exists
     */
    void addNativeMethod(const std::string& className, const std::string& methodName,
                        std::function<ValuePtr(const std::vector<ValuePtr>&)> implementation,
                        Variables::Type returnType = Variables::Type::NULL_TYPE,
                        const std::vector<ParameterInfo>& parameters = {},
                        bool isPrivate = false);

    /**
     * @brief Check if a class has a specific property
     * @param className Name of the class to check
     * @param propertyName Name of the property to check for
     * @return True if the property exists, false otherwise
     */
    bool hasProperty(const std::string& className, const std::string& propertyName) const;

    /**
     * @brief Check if a class has a specific method
     * @param className Name of the class to check
     * @param methodName Name of the method to check for
     * @return True if the method exists, false otherwise
     */
    bool hasMethod(const std::string& className, const std::string& methodName) const;

    /**
     * @brief Get a list of all registered class names
     * @return Vector of registered class names
     */
    std::vector<std::string> getClassNames() const;

    /**
     * @brief Get a property's type from a class
     * @param className Name of the class
     * @param propertyName Name of the property
     * @return Type of the property
     * @throws ClassException if the class or property is not found
     */
    Variables::Type getPropertyType(const std::string& className, const std::string& propertyName) const;

    /**
     * @brief Get a method's return type from a class
     * @param className Name of the class
     * @param methodName Name of the method
     * @return Return type of the method
     * @throws ClassException if the class or method is not found
     */
    Variables::Type getMethodReturnType(const std::string& className, const std::string& methodName) const;

    /**
     * @brief Get a method's parameters from a class
     * @param className Name of the class
     * @param methodName Name of the method
     * @return Vector of method parameters
     * @throws ClassException if the class or method is not found
     */
    const std::vector<ParameterInfo>& getMethodParameters(const std::string& className, const std::string& methodName) const;

    /**
     * @brief Set a static property value for a class
     * @param className Name of the class
     * @param propertyName Name of the property
     * @param value Value to set
     * @throws ClassException if the class is not registered
     */
    void setStaticProperty(const std::string& className, const std::string& propertyName, const ValuePtr& value);

    /**
     * @brief Get a static property value from a class
     * @param className Name of the class
     * @param propertyName Name of the property
     * @return Value of the property
     * @throws ClassException if the class or property is not found
     */
    ValuePtr getStaticProperty(const std::string& className, const std::string& propertyName) const;

    /**
     * @brief Check if a class has a specific static property
     * @param className Name of the class to check
     * @param propertyName Name of the property to check for
     * @return True if the property exists, false otherwise
     */
    bool hasStaticProperty(const std::string& className, const std::string& propertyName) const;

    /**
     * @brief Delete a static property from a class
     * @param className Name of the class
     * @param propertyName Name of the property to delete
     * @throws ClassException if the class is not registered
     */
    void deleteStaticProperty(const std::string& className, const std::string& propertyName);

    /**
     * @brief Clear all static properties from a class
     * @param className Name of the class
     * @throws ClassException if the class is not registered
     */
    void clearStaticProperties(const std::string& className);

    /**
     * @brief Get the module that defined a class
     * @param className Name of the class
     * @return Pointer to the module, or nullptr for script-defined classes
     * @throws ClassException if the class is not registered
     */
    Modules::BaseModule* getClassModule(const std::string& className) const;

    /**
     * @brief Find a property in a class hierarchy
     * @param className Name of the class to start searching from
     * @param propertyName Name of the property to find
     * @return Pointer to the PropertyInfo if found, nullptr otherwise
     */
    const PropertyInfo* findProperty(const std::string& className, const std::string& propertyName) const;

    /**
     * @brief Find a method in a class hierarchy
     * @param className Name of the class to start searching from
     * @param methodName Name of the method to find
     * @return Pointer to the MethodInfo if found, nullptr otherwise
     */
    const MethodInfo* findMethod(const std::string& className, const std::string& methodName) const;

    /**
     * @brief Call a method on a class instance
     * @param className Name of the class
     * @param methodName Name of the method
     * @param args Method arguments
     * @return Method return value
     * @throws ClassException if the class or method is not found
     */
    ValuePtr callMethod(const std::string& className, const std::string& methodName, 
                       const std::vector<ValuePtr>& args) const;

private:
    // Map of class name to class information
    std::unordered_map<std::string, ClassInfo> classes_;

    // Empty parameter vector for getMethodParameters
    static const std::vector<ParameterInfo> empty_parameters_;

    /**
     * @brief Helper method to find a class info or throw an exception
     * @param className Name of the class to find
     * @param errorMsg Error message to use if not found
     * @return Reference to the ClassInfo
     * @throws ClassException if the class is not found
     */
    ClassInfo& findOrThrow(const std::string& className, const std::string& errorMsg);

    /**
     * @brief Helper method to find a class info or throw an exception (const version)
     * @param className Name of the class to find
     * @param errorMsg Error message to use if not found
     * @return Const reference to the ClassInfo
     * @throws ClassException if the class is not found
     */
    const ClassInfo& findOrThrow(const std::string& className, const std::string& errorMsg) const;

    /**
     * @brief Helper method to find a property in a class
     * @param className Name of the class
     * @param propertyName Name of the property
     * @return Iterator to the property if found, end iterator otherwise
     */
    std::vector<PropertyInfo>::const_iterator findPropertyInClass(const ClassInfo& classInfo, 
                                                                const std::string& propertyName) const;

    /**
     * @brief Helper method to find a method in a class
     * @param className Name of the class
     * @param methodName Name of the method
     * @return Iterator to the method if found, end iterator otherwise
     */
    std::vector<MethodInfo>::const_iterator findMethodInClass(const ClassInfo& classInfo, 
                                                            const std::string& methodName) const;
};

} // namespace Symbols

#endif // SYMBOL_UNIFIED_CLASS_CONTAINER_HPP
