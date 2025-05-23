#ifndef SYMBOL_CLASS_REGISTRY_HPP
#define SYMBOL_CLASS_REGISTRY_HPP

#include "Symbols/UnifiedClassContainer.hpp"
#include "Symbols/ClassFactory.hpp"
#include <memory>

namespace Symbols {

/**
 * @brief Registry for class definitions and instances
 * 
 * This class provides a unified interface for registering classes,
 * creating instances, and managing class properties and methods.
 */
class ClassRegistry {
public:
    /**
     * @brief Constructor
     */
    ClassRegistry();

    /**
     * @brief Destructor
     */
    ~ClassRegistry();

    /**
     * @brief Get the class container
     * @return Reference to the class container
     */
    UnifiedClassContainer& getClassContainer();

    /**
     * @brief Get the class factory
     * @return Reference to the class factory
     */
    ClassFactory& getClassFactory();

    /**
     * @brief Register a new class
     * @param className Name of the class to register
     * @param module Pointer to the module that defines this class (nullptr for script-defined classes)
     * @return Reference to the newly created ClassInfo
     */
    UnifiedClassContainer::ClassInfo& registerClass(const std::string& className, Modules::BaseModule* module = nullptr);

    /**
     * @brief Register a new class with inheritance
     * @param className Name of the class to register
     * @param parentClassName Name of the parent class
     * @param module Pointer to the module that defines this class (nullptr for script-defined classes)
     * @return Reference to the newly created ClassInfo
     */
    UnifiedClassContainer::ClassInfo& registerClass(const std::string& className, 
                                                  const std::string& parentClassName, 
                                                  Modules::BaseModule* module = nullptr);

    /**
     * @brief Check if a class is registered
     * @param className Name of the class to check
     * @return True if the class is registered, false otherwise
     */
    bool hasClass(const std::string& className) const;

    /**
     * @brief Create a new instance of a class
     * @param className Name of the class to instantiate
     * @param constructorArgs Arguments to pass to the constructor
     * @return Value pointer to the new instance
     */
    ValuePtr createInstance(const std::string& className, const std::vector<ValuePtr>& constructorArgs = {});

    /**
     * @brief Get a static property value from a class
     * @param className Name of the class
     * @param propertyName Name of the property
     * @return Value of the property
     */
    ValuePtr getStaticProperty(const std::string& className, const std::string& propertyName) const;

    /**
     * @brief Set a static property value for a class
     * @param className Name of the class
     * @param propertyName Name of the property
     * @param value Value to set
     */
    void setStaticProperty(const std::string& className, const std::string& propertyName, const ValuePtr& value);

    /**
     * @brief Check if a class has a specific static property
     * @param className Name of the class to check
     * @param propertyName Name of the property to check for
     * @return True if the property exists, false otherwise
     */
    bool hasStaticProperty(const std::string& className, const std::string& propertyName) const;

    /**
     * @brief Get a property value from a class instance
     * @param instance Class instance to get the property from
     * @param propertyName Name of the property
     * @return Value of the property
     */
    ValuePtr getInstanceProperty(const ValuePtr& instance, const std::string& propertyName);

    /**
     * @brief Set a property value on a class instance
     * @param instance Class instance to set the property on
     * @param propertyName Name of the property
     * @param value Value to set
     */
    void setInstanceProperty(ValuePtr& instance, const std::string& propertyName, const ValuePtr& value);

    /**
     * @brief Call a method on a class instance
     * @param instance Class instance to call the method on
     * @param methodName Name of the method
     * @param args Arguments to pass to the method
     * @return Value returned by the method
     */
    ValuePtr callMethod(const ValuePtr& instance, const std::string& methodName, 
                      const std::vector<ValuePtr>& args = {});

    /**
     * @brief Check if an object is an instance of a specific class
     * @param instance Object to check
     * @param className Name of the class to check against
     * @return True if the object is an instance of the class, false otherwise
     */
    bool isInstanceOf(const ValuePtr& instance, const std::string& className);

    /**
     * @brief Get singleton instance of the class registry
     * @return Reference to the singleton instance
     */
    static ClassRegistry& instance();

private:
    // Singleton instance
    static ClassRegistry* instance_;

    // Class container
    std::unique_ptr<UnifiedClassContainer> classContainer_;

    // Class factory
    std::unique_ptr<ClassFactory> classFactory_;
};

} // namespace Symbols

#endif // SYMBOL_CLASS_REGISTRY_HPP
