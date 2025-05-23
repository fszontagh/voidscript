#ifndef SYMBOL_CLASS_CONTAINER_HPP
#define SYMBOL_CLASS_CONTAINER_HPP

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Parser/ParsedExpression.hpp"
#include "Symbols/ParameterContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Symbols {

/**
 * @brief Container for class definitions in the SoniScript language.
 * 
 * This class manages class definitions, their properties, methods, and inheritance.
 * It provides a singleton interface for global access to class information.
 */
class ClassContainer {
public:
    /**
     * @brief Class property information.
     */
    struct PropertyInfo {
        std::string name;
        Variables::Type type;
        Parser::ParsedExpressionPtr defaultValueExpr;
        bool isPrivate = false;
    };

    /**
     * @brief Class method information.
     */
    struct MethodInfo {
        std::string name;
        std::string qualifiedName;
        Variables::Type returnType;
        std::vector<functionParameterType> parameters;
        bool isPrivate = false;
    };

    /**
     * @brief Full class definition information.
     */
    struct ClassInfo {
        std::string name;
        std::string parentClass; // For inheritance
        std::vector<Symbols::ClassContainer::PropertyInfo> properties;
        std::vector<Symbols::ClassContainer::MethodInfo> methods;
        std::unordered_map<std::string, ValuePtr> objectProperties; // For static properties
        Modules::BaseModule* module = nullptr; // Module that defined this class
    };

    /**
     * @brief Exception class for ClassContainer errors.
     */
    class Exception : public std::runtime_error {
    public:
        explicit Exception(const std::string& message) : std::runtime_error(message) {}
    };

    // Delete copy and move constructors/assignments
    ClassContainer(const ClassContainer&) = delete;
    ClassContainer& operator=(const ClassContainer&) = delete;
    ClassContainer(ClassContainer&&) = delete;
    ClassContainer& operator=(ClassContainer&&) = delete;

    /**
     * @brief Get the singleton instance of the ClassContainer.
     * @return Pointer to the singleton instance.
     */
    static ClassContainer* instance();

    /**
     * @brief Register a new class.
     * @param className Name of the class to register.
     * @param module Pointer to the module that defines this class (nullptr for script-defined classes).
     * @return Reference to the newly created ClassInfo.
     */
    virtual Symbols::ClassContainer::ClassInfo& registerClass(const std::string& className, Modules::BaseModule* module = nullptr);

    /**
     * @brief Register a new class with inheritance.
     * @param className Name of the class to register.
     * @param parentClassName Name of the parent class.
     * @param module Pointer to the module that defines this class (nullptr for script-defined classes).
     * @return Reference to the newly created ClassInfo.
     */
    virtual Symbols::ClassContainer::ClassInfo& registerClass(const std::string& className, const std::string& parentClassName, Modules::BaseModule* module = nullptr);

    /**
     * @brief Check if a class is registered.
     * @param className Name of the class to check.
     * @return True if the class is registered, false otherwise.
     */
    virtual bool hasClass(const std::string& className) const;

    /**
     * @brief Get information about a registered class.
     * @param className Name of the class to get information for.
     * @return Reference to the ClassInfo for the class.
     * @throws Exception if the class is not registered.
     */
    virtual Symbols::ClassContainer::ClassInfo& getClassInfo(const std::string& className);

    /**
     * @brief Get information about a registered class (const version).
     * @param className Name of the class to get information for.
     * @return Const reference to the ClassInfo for the class.
     * @throws Exception if the class is not registered.
     */
    virtual const Symbols::ClassContainer::ClassInfo& getClassInfo(const std::string& className) const;

    /**
     * @brief Add a property to a class.
     * @param className Name of the class to add the property to.
     * @param propertyName Name of the property.
     * @param type Type of the property.
     * @param isPrivate Whether the property is private.
     * @param defaultValueExpr Expression for the default value (optional).
     * @throws Exception if the class is not registered.
     */
    virtual void addProperty(const std::string& className, const std::string& propertyName, 
                    Variables::Type type, bool isPrivate = false, 
                    Parser::ParsedExpressionPtr defaultValueExpr = nullptr);

    /**
     * @brief Add a method to a class.
     * @param className Name of the class to add the method to.
     * @param methodName Name of the method.
     * @param returnType Return type of the method.
     * @param parameters Method parameters.
     * @param isPrivate Whether the method is private.
     * @throws Exception if the class is not registered.
     */
    virtual void addMethod(const std::string& className, const std::string& methodName,
                  Variables::Type returnType = Variables::Type::NULL_TYPE,
                  const std::vector<functionParameterType>& parameters = {},
                  bool isPrivate = false);

    /**
     * @brief Check if a class has a specific property.
     * @param className Name of the class to check.
     * @param propertyName Name of the property to check for.
     * @return True if the property exists, false otherwise.
     */
    virtual bool hasProperty(const std::string& className, const std::string& propertyName) const;

    /**
     * @brief Check if a class has a specific method.
     * @param className Name of the class to check.
     * @param methodName Name of the method to check for.
     * @return True if the method exists, false otherwise.
     */
    virtual bool hasMethod(const std::string& className, const std::string& methodName) const;

    /**
     * @brief Get a list of all registered class names.
     * @return Vector of registered class names.
     */
    virtual std::vector<std::string> getClassNames() const;

    /**
     * @brief Get a property's type from a class.
     * @param className Name of the class.
     * @param propertyName Name of the property.
     * @return Type of the property.
     * @throws Exception if the class or property is not found.
     */
    virtual Variables::Type getPropertyType(const std::string& className, const std::string& propertyName) const;

    /**
     * @brief Get a method's return type from a class.
     * @param className Name of the class.
     * @param methodName Name of the method.
     * @return Return type of the method.
     * @throws Exception if the class or method is not found.
     */
    virtual Variables::Type getMethodReturnType(const std::string& className, const std::string& methodName) const;

    /**
     * @brief Get a method's parameters from a class.
     * @param className Name of the class.
     * @param methodName Name of the method.
     * @return Vector of parameter information.
     * @throws Exception if the class or method is not found.
     */
    virtual const std::vector<functionParameterType>& getMethodParameters(
        const std::string& className, const std::string& methodName) const;

    /**
     * @brief Set a static property value for a class.
     * @param className Name of the class.
     * @param propertyName Name of the property.
     * @param value Value to set.
     * @throws Exception if the class is not registered.
     */
    virtual void setObjectProperty(const std::string& className, const std::string& propertyName, const ValuePtr& value);

    /**
     * @brief Get a static property value from a class.
     * @param className Name of the class.
     * @param propertyName Name of the property.
     * @return Value of the property.
     * @throws Exception if the class or property is not found.
     */
    virtual ValuePtr getObjectProperty(const std::string& className, const std::string& propertyName) const;

    /**
     * @brief Check if a class has a specific static property.
     * @param className Name of the class to check.
     * @param propertyName Name of the property to check for.
     * @return True if the property exists, false otherwise.
     */
    virtual bool hasObjectProperty(const std::string& className, const std::string& propertyName) const;

    /**
     * @brief Delete a static property from a class.
     * @param className Name of the class.
     * @param propertyName Name of the property to delete.
     * @throws Exception if the class is not registered.
     */
    virtual void deleteObjectProperty(const std::string& className, const std::string& propertyName);

    /**
     * @brief Clear all static properties from a class.
     * @param className Name of the class.
     * @throws Exception if the class is not registered.
     */
    virtual void clearObjectProperties(const std::string& className);

    /**
     * @brief Get the module that defined a class.
     * @param className Name of the class.
     * @return Pointer to the module, or nullptr for script-defined classes.
     * @throws Exception if the class is not registered.
     */
    virtual Modules::BaseModule* getClassModule(const std::string& className) const;

protected:
    // Protected constructor for singleton
    ClassContainer() = default;

private:
    // Singleton instance
    static ClassContainer* instance_;

    // Map of class name to class information
    std::unordered_map<std::string, Symbols::ClassContainer::ClassInfo> classes_;

    // Empty parameter vector for getMethodParameters
    static const std::vector<functionParameterType> empty_parameters_;

    /**
     * @brief Helper method to find a class info or throw an exception.
     * @param className Name of the class to find.
     * @param errorMsg Error message to use if not found.
     * @return Reference to the ClassInfo.
     * @throws Exception if the class is not found.
     */
    Symbols::ClassContainer::ClassInfo& findOrThrow(const std::string& className, const std::string& errorMsg);

    /**
     * @brief Helper method to find a class info or throw an exception (const version).
     * @param className Name of the class to find.
     * @param errorMsg Error message to use if not found.
     * @return Const reference to the ClassInfo.
     * @throws Exception if the class is not found.
     */
    const Symbols::ClassContainer::ClassInfo& findOrThrow(const std::string& className, const std::string& errorMsg) const;

    /**
     * @brief Find a property in a class, including inherited properties.
     * @param className Name of the class to search in.
     * @param propertyName Name of the property to find.
     * @return Pointer to the PropertyInfo, or nullptr if not found.
     */
    const Symbols::ClassContainer::PropertyInfo* findProperty(const std::string& className, const std::string& propertyName) const;

    /**
     * @brief Find a method in a class, including inherited methods.
     * @param className Name of the class to search in.
     * @param methodName Name of the method to find.
     * @return Pointer to the MethodInfo, or nullptr if not found.
     */
    const Symbols::ClassContainer::MethodInfo* findMethod(const std::string& className, const std::string& methodName) const;
};

} // namespace Symbols

#endif // SYMBOL_CLASS_CONTAINER_HPP
