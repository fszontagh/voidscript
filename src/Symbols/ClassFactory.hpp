#ifndef SYMBOL_CLASS_FACTORY_HPP
#define SYMBOL_CLASS_FACTORY_HPP

#include "Symbols/UnifiedClassContainer.hpp"
#include "Symbols/Value.hpp"
#include <string>
#include <vector>

namespace Symbols {

/**
 * @brief Factory class for creating and managing class instances
 * 
 * This class provides methods for creating instances of classes, 
 * calling methods on instances, and managing class properties.
 */
class ClassFactory {
public:
    /**
     * @brief Constructor
     * @param classContainer Reference to the class container to use
     */
    explicit ClassFactory(UnifiedClassContainer& classContainer);

    /**
     * @brief Create a new instance of a class
     * @param className Name of the class to instantiate
     * @param constructorArgs Arguments to pass to the constructor
     * @return Value pointer to the new instance
     * @throws ClassException if the class doesn't exist or constructor fails
     */
    ValuePtr createInstance(const std::string& className, const std::vector<ValuePtr>& constructorArgs = {});

    /**
     * @brief Call a method on a class instance
     * @param instance Class instance to call the method on
     * @param methodName Name of the method to call
     * @param args Arguments to pass to the method
     * @return Value returned by the method
     * @throws ClassException if the instance is invalid or method doesn't exist
     */
    ValuePtr callMethod(const ValuePtr& instance, const std::string& methodName, 
                      const std::vector<ValuePtr>& args = {});

    /**
     * @brief Get a property from a class instance
     * @param instance Class instance to get the property from
     * @param propertyName Name of the property to get
     * @return Value of the property
     * @throws ClassException if the instance is invalid or property doesn't exist
     */
    ValuePtr getProperty(const ValuePtr& instance, const std::string& propertyName);

    /**
     * @brief Set a property on a class instance
     * @param instance Class instance to set the property on
     * @param propertyName Name of the property to set
     * @param value Value to set the property to
     * @throws ClassException if the instance is invalid or property doesn't exist
     */
    void setProperty(ValuePtr& instance, const std::string& propertyName, const ValuePtr& value);

    /**
     * @brief Check if an instance has a property
     * @param instance Class instance to check
     * @param propertyName Name of the property to check for
     * @return True if the instance has the property, false otherwise
     */
    bool hasProperty(const ValuePtr& instance, const std::string& propertyName);

    /**
     * @brief Check if an instance has a method
     * @param instance Class instance to check
     * @param methodName Name of the method to check for
     * @return True if the instance has the method, false otherwise
     */
    bool hasMethod(const ValuePtr& instance, const std::string& methodName);

    /**
     * @brief Get the class name of an instance
     * @param instance Class instance to get the class name from
     * @return Name of the class
     * @throws ClassException if the instance is invalid
     */
    std::string getClassName(const ValuePtr& instance);

    /**
     * @brief Check if an object is an instance of a specific class
     * @param instance Object to check
     * @param className Name of the class to check against
     * @return True if the object is an instance of the class, false otherwise
     */
    bool isInstanceOf(const ValuePtr& instance, const std::string& className);

private:
    // Reference to the class container
    UnifiedClassContainer& classContainer_;

    /**
     * @brief Initialize a new class instance with default property values
     * @param instance Instance to initialize
     * @param className Name of the class
     * @throws ClassException if initialization fails
     */
    void initializeInstance(ValuePtr& instance, const std::string& className);

    /**
     * @brief Get a metadata property from an instance
     * @param instance Instance to get metadata from
     * @param metaKey Metadata key to get
     * @return Value of the metadata or null if not found
     * @note This is a static utility method.
     */
    static ValuePtr getInstanceMetadata(const ValuePtr& instance, const std::string& metaKey);

    /**
     * @brief Set a metadata property on an instance
     * @param instance Instance to set metadata on
     * @param metaKey Metadata key to set
     * @param value Value to set
     * @note This is a static utility method.
     */
    static void setInstanceMetadata(ValuePtr& instance, const std::string& metaKey, const ValuePtr& value);

    /**
     * @brief Create an object map for a class instance
     * @param className Name of the class
     * @return Object map with initialized metadata
     * @note This is a static utility method.
     */
    static ObjectMap createInstanceMap(const std::string& className);
};

} // namespace Symbols

#endif // SYMBOL_CLASS_FACTORY_HPP
