#include "Symbols/ClassFactory.hpp"

namespace Symbols {

ClassFactory::ClassFactory(UnifiedClassContainer& classContainer)
    : classContainer_(classContainer) {
}

ValuePtr ClassFactory::createInstance(const std::string& className, const std::vector<ValuePtr>& constructorArgs) {
    // Check if the class exists
    if (!classContainer_.hasClass(className)) {
        throw UnifiedClassContainer::ClassException("Class not found: " + className);
    }

    // Create the instance map and set __class__ metadata
    ObjectMap instanceMap = createInstanceMap(className);
    // Use makeClassInstance to ensure correct type and initialization
    auto instance = ValuePtr::makeClassInstance(instanceMap);

    // Initialize properties with default values
    initializeInstance(instance, className);

    // Call constructor if available and args provided
    if (classContainer_.hasMethod(className, "construct") && !constructorArgs.empty()) {
        callMethod(instance, "construct", constructorArgs);
    }

    return instance;
}

ValuePtr ClassFactory::callMethod(const ValuePtr& instance, const std::string& methodName, 
                                const std::vector<ValuePtr>& args) {
    // Check if the instance is a class instance
    if (instance->getType() != Variables::Type::OBJECT) {
        throw UnifiedClassContainer::ClassException("Cannot call method on non-object");
    }

    // Get the class name
    std::string className = getClassName(instance);
    if (className.empty()) {
        throw UnifiedClassContainer::ClassException("Invalid class instance");
    }

    // Check if the method exists
    if (!classContainer_.hasMethod(className, methodName)) {
        throw UnifiedClassContainer::ClassException("Method not found: " + methodName);
    }

    // Get the method
    const auto* methodInfo = classContainer_.findMethod(className, methodName);
    if (!methodInfo) {
        throw UnifiedClassContainer::ClassException("Method not found: " + methodName);
    }

    // If it's a native method, call it directly
    if (methodInfo->nativeImplementation) {
        // Prepend the instance to the arguments
        std::vector<ValuePtr> methodArgs = {instance};
        methodArgs.insert(methodArgs.end(), args.begin(), args.end());
        return methodInfo->nativeImplementation(methodArgs);
    }

    // For script methods, the interpreter needs to handle the call
    // This is a placeholder for now - the actual implementation would depend on the interpreter
    throw UnifiedClassContainer::ClassException("Script method calls not implemented in ClassFactory");
}

ValuePtr ClassFactory::getProperty(const ValuePtr& instance, const std::string& propertyName) {
    if (instance->getType() != Variables::Type::OBJECT && instance->getType() != Variables::Type::CLASS) {
        throw UnifiedClassContainer::ClassException("Cannot get property from non-object");
    }
    const ObjectMap& map = instance->get<ObjectMap>();
    auto it = map.find(propertyName);
    if (it != map.end()) {
        return it->second;
    }
    // If not found, check if it's a property in the class definition
    std::string className = getClassName(instance);
    if (!className.empty() && classContainer_.hasProperty(className, propertyName)) {
        const auto* propInfo = classContainer_.findProperty(className, propertyName);
        if (propInfo && propInfo->isPrivate) {
            throw UnifiedClassContainer::ClassException("Cannot access private property: " + propertyName);
        }
        return ValuePtr::null(propInfo->type);
    }
    throw UnifiedClassContainer::ClassException("Property not found: " + propertyName);
}

void ClassFactory::setProperty(ValuePtr& instance, const std::string& propertyName, const ValuePtr& value) {
    if (instance->getType() != Variables::Type::OBJECT && instance->getType() != Variables::Type::CLASS) {
        throw UnifiedClassContainer::ClassException("Cannot set property on non-object");
    }
    std::string className = getClassName(instance);
    if (className.empty()) {
        throw UnifiedClassContainer::ClassException("Invalid class instance");
    }
    if (classContainer_.hasProperty(className, propertyName)) {
        const auto* propInfo = classContainer_.findProperty(className, propertyName);
        if (propInfo && propInfo->isPrivate) {
            throw UnifiedClassContainer::ClassException("Cannot access private property: " + propertyName);
        }
        if (propInfo && value->getType() != propInfo->type && value->getType() != Variables::Type::NULL_TYPE) {
            throw UnifiedClassContainer::ClassException(
                "Type mismatch for property " + propertyName +
                ": expected " + Variables::TypeToString(propInfo->type) +
                ", got " + Variables::TypeToString(value->getType())
            );
        }
    }
    ObjectMap& map = instance->get<ObjectMap>();
    map[propertyName] = value;
}

bool ClassFactory::hasProperty(const ValuePtr& instance, const std::string& propertyName) {
    if (instance->getType() != Variables::Type::OBJECT && instance->getType() != Variables::Type::CLASS) {
        return false;
    }
    const ObjectMap& map = instance->get<ObjectMap>();
    if (map.find(propertyName) != map.end()) {
        return true;
    }
    std::string className = getClassName(instance);
    if (!className.empty()) {
        return classContainer_.hasProperty(className, propertyName);
    }
    return false;
}

bool ClassFactory::hasMethod(const ValuePtr& instance, const std::string& methodName) {
    // Check if the instance is a class instance
    if (instance->getType() != Variables::Type::OBJECT) {
        return false;
    }

    // Get the class name
    std::string className = getClassName(instance);
    if (className.empty()) {
        return false;
    }

    // Check if the method exists in the class
    return classContainer_.hasMethod(className, methodName);
}

std::string ClassFactory::getClassName(const ValuePtr& instance) {
    // Check if the instance is a class instance
    if (instance->getType() != Variables::Type::OBJECT) {
        return "";
    }

    // Get the class name from metadata
    auto className = getInstanceMetadata(instance, "__class__");
    if (className->getType() != Variables::Type::STRING) {
        return "";
    }

    return className->get<std::string>();
}

bool ClassFactory::isInstanceOf(const ValuePtr& instance, const std::string& className) {
    // Check if the instance is a class instance
    if (instance->getType() != Variables::Type::OBJECT) {
        return false;
    }

    // Get the instance's class name
    std::string instanceClassName = getClassName(instance);
    if (instanceClassName.empty()) {
        return false;
    }

    // Check if it's an instance of the specified class
    if (instanceClassName == className) {
        return true;
    }

    // Check the inheritance chain
    try {
        std::string parentClass = classContainer_.getClassInfo(instanceClassName).parentClass;
        while (!parentClass.empty()) {
            if (parentClass == className) {
                return true;
            }
            parentClass = classContainer_.getClassInfo(parentClass).parentClass;
        }
    } catch (const UnifiedClassContainer::ClassException&) {
        // If we can't find the class info, it's not an instance of the class
        return false;
    }

    return false;
}

void ClassFactory::initializeInstance(ValuePtr& instance, const std::string& className) {
    const auto& classInfo = classContainer_.getClassInfo(className);
    ObjectMap& map = instance->get<ObjectMap>();
    for (const auto& prop : classInfo.properties) {
        if (map.find(prop.name) != map.end()) {
            continue;
        }
        if (prop.defaultValueExpr) {
            // TODO: evaluate defaultValueExpr - for now use type defaults
            switch (prop.type) {
                case Variables::Type::INTEGER:
                    map[prop.name] = ValuePtr(0);
                    break;
                case Variables::Type::DOUBLE:
                    map[prop.name] = ValuePtr(0.0);
                    break;
                case Variables::Type::FLOAT:
                    map[prop.name] = ValuePtr(0.0f);
                    break;
                case Variables::Type::STRING:
                    map[prop.name] = ValuePtr("");
                    break;
                case Variables::Type::BOOLEAN:
                    map[prop.name] = ValuePtr(false);
                    break;
                case Variables::Type::OBJECT:
                    map[prop.name] = ValuePtr(ObjectMap());
                    break;
                default:
                    map[prop.name] = ValuePtr::null(prop.type);
                    break;
            }
        } else {
            // Create default value for the property type instead of null
            switch (prop.type) {
                case Variables::Type::INTEGER:
                    map[prop.name] = ValuePtr(0);
                    break;
                case Variables::Type::DOUBLE:
                    map[prop.name] = ValuePtr(0.0);
                    break;
                case Variables::Type::FLOAT:
                    map[prop.name] = ValuePtr(0.0f);
                    break;
                case Variables::Type::STRING:
                    map[prop.name] = ValuePtr("");
                    break;
                case Variables::Type::BOOLEAN:
                    map[prop.name] = ValuePtr(false);
                    break;
                case Variables::Type::OBJECT:
                    map[prop.name] = ValuePtr(ObjectMap());
                    break;
                default:
                    map[prop.name] = ValuePtr::null(prop.type);
                    break;
            }
        }
    }
    if (!classInfo.parentClass.empty()) {
        initializeInstance(instance, classInfo.parentClass);
    }
}

ValuePtr ClassFactory::getInstanceMetadata(const ValuePtr& instance, const std::string& metaKey) {
    if (instance->getType() != Variables::Type::OBJECT && instance->getType() != Variables::Type::CLASS) {
        return ValuePtr::null();
    }
    const ObjectMap& map = instance->get<ObjectMap>();
    auto it = map.find("__metadata__");
    if (it == map.end() || it->second->getType() != Variables::Type::OBJECT) {
        return ValuePtr::null();
    }
    const ObjectMap& metaMap = it->second->get<ObjectMap>();
    auto mit = metaMap.find(metaKey);
    if (mit != metaMap.end()) {
        return mit->second;
    }
    return ValuePtr::null();
}

void ClassFactory::setInstanceMetadata(ValuePtr& instance, const std::string& metaKey, const ValuePtr& value) {
    if (instance->getType() != Variables::Type::OBJECT && instance->getType() != Variables::Type::CLASS) {
        throw UnifiedClassContainer::ClassException("Cannot set metadata on non-object");
    }
    ObjectMap& map = instance->get<ObjectMap>();
    if (map.find("__metadata__") == map.end() || map["__metadata__"]->getType() != Variables::Type::OBJECT) {
        map["__metadata__"] = ValuePtr(ObjectMap());
    }
    ObjectMap& metaMap = map["__metadata__"]->get<ObjectMap>();
    metaMap[metaKey] = value;
}

ObjectMap ClassFactory::createInstanceMap(const std::string& className) {
    ObjectMap instanceMap;
    ObjectMap metadataMap;
    metadataMap["__class__"] = ValuePtr(className);
    instanceMap["__metadata__"] = ValuePtr(metadataMap);
    return instanceMap;
}

} // namespace Symbols
