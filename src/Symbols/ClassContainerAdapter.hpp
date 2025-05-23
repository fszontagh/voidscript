#ifndef SYMBOL_CLASS_CONTAINER_ADAPTER_HPP
#define SYMBOL_CLASS_CONTAINER_ADAPTER_HPP

#include "Symbols/ClassContainer.hpp"
#include "Symbols/ClassRegistry.hpp"
#include <memory>

namespace Symbols {

/**
 * @brief Adapter for ClassContainer to use the new ClassRegistry.
 * 
 * This class maintains the existing ClassContainer interface but delegates
 * all operations to the new ClassRegistry. This allows for a gradual
 * migration to the new class system without breaking existing code.
 */
class ClassContainerAdapter : public ClassContainer {
public:
    /**
     * @brief Get the singleton instance of the ClassContainerAdapter.
     * @return Pointer to the singleton instance.
     */
    static ClassContainerAdapter* instance();

    /**
     * @brief Register a new class.
     * @param className Name of the class to register.
     * @param module Pointer to the module that defines this class (nullptr for script-defined classes).
     * @return Reference to the newly created ClassInfo.
     */
    ClassInfo& registerClass(const std::string& className, Modules::BaseModule* module = nullptr) override;

    /**
     * @brief Register a new class with inheritance.
     * @param className Name of the class to register.
     * @param parentClassName Name of the parent class.
     * @param module Pointer to the module that defines this class (nullptr for script-defined classes).
     * @return Reference to the newly created ClassInfo.
     */
    ClassInfo& registerClass(const std::string& className, const std::string& parentClassName, 
                           Modules::BaseModule* module = nullptr) override;

    /**
     * @brief Check if a class is registered.
     * @param className Name of the class to check.
     * @return True if the class is registered, false otherwise.
     */
    bool hasClass(const std::string& className) const override;

    /**
     * @brief Get information about a registered class.
     * @param className Name of the class to get information for.
     * @return Reference to the ClassInfo for the class.
     * @throws Exception if the class is not registered.
     */
    ClassInfo& getClassInfo(const std::string& className) override;

    /**
     * @brief Get information about a registered class (const version).
     * @param className Name of the class to get information for.
     * @return Const reference to the ClassInfo for the class.
     * @throws Exception if the class is not registered.
     */
    const ClassInfo& getClassInfo(const std::string& className) const override;

    /**
     * @brief Add a property to a class.
     * @param className Name of the class to add the property to.
     * @param propertyName Name of the property.
     * @param type Type of the property.
     * @param isPrivate Whether the property is private.
     * @param defaultValueExpr Expression for the default value (optional).
     * @throws Exception if the class is not registered.
     */
    void addProperty(const std::string& className, const std::string& propertyName, 
                    Variables::Type type, bool isPrivate = false, 
                    Parser::ParsedExpressionPtr defaultValueExpr = nullptr) override;

    /**
     * @brief Add a method to a class.
     * @param className Name of the class to add the method to.
     * @param methodName Name of the method.
     * @param returnType Return type of the method.
     * @param parameters Method parameters.
     * @param isPrivate Whether the method is private.
     * @throws Exception if the class is not registered.
     */
    void addMethod(const std::string& className, const std::string& methodName,
                  Variables::Type returnType = Variables::Type::NULL_TYPE,
                  const std::vector<functionParameterType>& parameters = {},
                  bool isPrivate = false) override;

    /**
     * @brief Check if a class has a specific property.
     * @param className Name of the class to check.
     * @param propertyName Name of the property to check for.
     * @return True if the property exists, false otherwise.
     */
    bool hasProperty(const std::string& className, const std::string& propertyName) const override;

    /**
     * @brief Check if a class has a specific method.
     * @param className Name of the class to check.
     * @param methodName Name of the method to check for.
     * @return True if the method exists, false otherwise.
     */
    bool hasMethod(const std::string& className, const std::string& methodName) const override;

    /**
     * @brief Get a list of all registered class names.
     * @return Vector of registered class names.
     */
    std::vector<std::string> getClassNames() const override;

    /**
     * @brief Get a property's type from a class.
     * @param className Name of the class.
     * @param propertyName Name of the property.
     * @return Type of the property.
     * @throws Exception if the class or property is not found.
     */
    Variables::Type getPropertyType(const std::string& className, 
                                  const std::string& propertyName) const override;

    /**
     * @brief Get a method's return type from a class.
     * @param className Name of the class.
     * @param methodName Name of the method.
     * @return Return type of the method.
     * @throws Exception if the class or method is not found.
     */
    Variables::Type getMethodReturnType(const std::string& className, 
                                      const std::string& methodName) const override;

    /**
     * @brief Get a method's parameters from a class.
     * @param className Name of the class.
     * @param methodName Name of the method.
     * @return Vector of method parameters.
     * @throws Exception if the class or method is not found.
     */
    const std::vector<functionParameterType>& getMethodParameters(
        const std::string& className, const std::string& methodName) const override;

    /**
     * @brief Set a static property value for a class.
     * @param className Name of the class.
     * @param propertyName Name of the property.
     * @param value Value to set.
     * @throws Exception if the class is not registered.
     */
    void setObjectProperty(const std::string& className, const std::string& propertyName, 
                          const ValuePtr& value) override;

    /**
     * @brief Get a static property value from a class.
     * @param className Name of the class.
     * @param propertyName Name of the property.
     * @return Value of the property.
     * @throws Exception if the class or property is not found.
     */
    ValuePtr getObjectProperty(const std::string& className, 
                             const std::string& propertyName) const override;

    /**
     * @brief Check if a class has a specific static property.
     * @param className Name of the class to check.
     * @param propertyName Name of the property to check for.
     * @return True if the property exists, false otherwise.
     */
    bool hasObjectProperty(const std::string& className, 
                          const std::string& propertyName) const override;

    /**
     * @brief Delete a static property from a class.
     * @param className Name of the class.
     * @param propertyName Name of the property to delete.
     * @throws Exception if the class is not registered.
     */
    void deleteObjectProperty(const std::string& className, 
                            const std::string& propertyName) override;

    /**
     * @brief Clear all static properties from a class.
     * @param className Name of the class.
     * @throws Exception if the class is not registered.
     */
    void clearObjectProperties(const std::string& className) override;

    /**
     * @brief Get the module that defined a class.
     * @param className Name of the class.
     * @return Pointer to the module, or nullptr for script-defined classes.
     * @throws Exception if the class is not registered.
     */
    Modules::BaseModule* getClassModule(const std::string& className) const override;

public:
    ClassContainerAdapter();

private:
    // Private constructor for singleton
    // ClassContainerAdapter();

    // Singleton instance
    static ClassContainerAdapter* instance_;

    // Map of class info proxies
    std::unordered_map<std::string, ClassInfo> classInfoProxies_;

    // Map of method parameters
    std::unordered_map<std::string, std::vector<functionParameterType>> methodParameters_;

    // Convert from ParameterInfo to functionParameterType
    functionParameterType convertParameterInfo(const UnifiedClassContainer::ParameterInfo& info) const;

    // Convert from functionParameterType to ParameterInfo
    UnifiedClassContainer::ParameterInfo convertFunctionParameter(const functionParameterType& param) const;
};

} // namespace Symbols

#endif // SYMBOL_CLASS_CONTAINER_ADAPTER_HPP
