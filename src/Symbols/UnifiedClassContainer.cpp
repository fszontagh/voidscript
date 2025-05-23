#include "Symbols/UnifiedClassContainer.hpp"
#include <algorithm>
#include <sstream>

namespace Symbols {

// Initialize static members
const std::vector<UnifiedClassContainer::ParameterInfo> UnifiedClassContainer::empty_parameters_ = {};

UnifiedClassContainer::UnifiedClassContainer() = default;

UnifiedClassContainer::~UnifiedClassContainer() = default;

UnifiedClassContainer::ClassInfo& UnifiedClassContainer::registerClass(
    const std::string& className, Modules::BaseModule* module) {
    if (hasClass(className)) {
        throw ClassException("Class already registered: " + className);
    }

    classes_[className] = ClassInfo{
        .name = className,
        .parentClass = "",
        .properties = {},
        .methods = {},
        .staticProperties = {},
        .module = module
    };

    return classes_[className];
}

UnifiedClassContainer::ClassInfo& UnifiedClassContainer::registerClass(
    const std::string& className, const std::string& parentClassName, Modules::BaseModule* module) {
    if (hasClass(className)) {
        throw ClassException("Class already registered: " + className);
    }

    if (!parentClassName.empty() && !hasClass(parentClassName)) {
        throw ClassException("Parent class not registered: " + parentClassName);
    }

    classes_[className] = ClassInfo{
        .name = className,
        .parentClass = parentClassName,
        .properties = {},
        .methods = {},
        .staticProperties = {},
        .module = module
    };

    return classes_[className];
}

bool UnifiedClassContainer::hasClass(const std::string& className) const {
    return classes_.find(className) != classes_.end();
}

UnifiedClassContainer::ClassInfo& UnifiedClassContainer::getClassInfo(const std::string& className) {
    return findOrThrow(className, "Class not found");
}

const UnifiedClassContainer::ClassInfo& UnifiedClassContainer::getClassInfo(const std::string& className) const {
    return findOrThrow(className, "Class not found");
}

void UnifiedClassContainer::addProperty(
    const std::string& className, const std::string& propertyName, 
    Variables::Type type, bool isPrivate, Parser::ParsedExpressionPtr defaultValueExpr) {
    ClassInfo& cls = findOrThrow(className, "Class not found");
    
    // Check if property already exists
    for (const auto& prop : cls.properties) {
        if (prop.name == propertyName) {
            throw ClassException("Property already exists in class: " + propertyName);
        }
    }
    
    cls.properties.push_back(PropertyInfo{
        .name = propertyName,
        .type = type,
        .defaultValueExpr = std::move(defaultValueExpr),
        .isPrivate = isPrivate
    });
}

void UnifiedClassContainer::addMethod(
    const std::string& className, const std::string& methodName,
    Variables::Type returnType, const std::vector<ParameterInfo>& parameters, bool isPrivate) {
    ClassInfo& cls = findOrThrow(className, "Class not found");
    
    // Check if method already exists
    for (const auto& method : cls.methods) {
        if (method.name == methodName) {
            throw ClassException("Method already exists in class: " + methodName);
        }
    }

    std::string qualifiedName = className + "::" + methodName;

    cls.methods.push_back(MethodInfo{
        .name = methodName,
        .qualifiedName = qualifiedName,
        .returnType = returnType,
        .parameters = parameters,
        .isPrivate = isPrivate,
        .nativeImplementation = nullptr // No native implementation for script methods
    });
}

void UnifiedClassContainer::addNativeMethod(
    const std::string& className, const std::string& methodName,
    std::function<ValuePtr(const std::vector<ValuePtr>&)> implementation,
    Variables::Type returnType, const std::vector<ParameterInfo>& parameters, bool isPrivate) {
    ClassInfo& cls = findOrThrow(className, "Class not found");
    
    // Check if method already exists
    for (const auto& method : cls.methods) {
        if (method.name == methodName) {
            throw ClassException("Method already exists in class: " + methodName);
        }
    }

    std::string qualifiedName = className + "::" + methodName;

    cls.methods.push_back(MethodInfo{
        .name = methodName,
        .qualifiedName = qualifiedName,
        .returnType = returnType,
        .parameters = parameters,
        .isPrivate = isPrivate,
        .nativeImplementation = implementation
    });
}

bool UnifiedClassContainer::hasProperty(const std::string& className, const std::string& propertyName) const {
    return findProperty(className, propertyName) != nullptr;
}

bool UnifiedClassContainer::hasMethod(const std::string& className, const std::string& methodName) const {
    return findMethod(className, methodName) != nullptr;
}

std::vector<std::string> UnifiedClassContainer::getClassNames() const {
    std::vector<std::string> names;
    names.reserve(classes_.size());
    
    for (const auto& pair : classes_) {
        names.push_back(pair.first);
    }
    
    return names;
}

Variables::Type UnifiedClassContainer::getPropertyType(
    const std::string& className, const std::string& propertyName) const {
    const PropertyInfo* prop = findProperty(className, propertyName);
    if (!prop) {
        throw ClassException("Property not found in class: " + propertyName);
    }
    
    return prop->type;
}

Variables::Type UnifiedClassContainer::getMethodReturnType(
    const std::string& className, const std::string& methodName) const {
    const MethodInfo* method = findMethod(className, methodName);
    if (!method) {
        throw ClassException("Method not found in class: " + methodName);
    }
    
    return method->returnType;
}

const std::vector<UnifiedClassContainer::ParameterInfo>& UnifiedClassContainer::getMethodParameters(
    const std::string& className, const std::string& methodName) const {
    const MethodInfo* method = findMethod(className, methodName);
    if (!method) {
        throw ClassException("Method not found in class: " + methodName);
    }
    
    return method->parameters;
}

void UnifiedClassContainer::setStaticProperty(
    const std::string& className, const std::string& propertyName, const ValuePtr& value) {
    ClassInfo& cls = findOrThrow(className, "Class not found");
    cls.staticProperties[propertyName] = value;
}

ValuePtr UnifiedClassContainer::getStaticProperty(
    const std::string& className, const std::string& propertyName) const {
    const ClassInfo& cls = findOrThrow(className, "Class not found");
    
    auto it = cls.staticProperties.find(propertyName);
    if (it == cls.staticProperties.end()) {
        throw ClassException("Static property not found in class: " + propertyName);
    }
    
    return it->second;
}

bool UnifiedClassContainer::hasStaticProperty(
    const std::string& className, const std::string& propertyName) const {
    if (!hasClass(className)) {
        return false;
    }
    
    const ClassInfo& cls = classes_.at(className);
    return cls.staticProperties.find(propertyName) != cls.staticProperties.end();
}

void UnifiedClassContainer::deleteStaticProperty(
    const std::string& className, const std::string& propertyName) {
    ClassInfo& cls = findOrThrow(className, "Class not found");
    cls.staticProperties.erase(propertyName);
}

void UnifiedClassContainer::clearStaticProperties(const std::string& className) {
    ClassInfo& cls = findOrThrow(className, "Class not found");
    cls.staticProperties.clear();
}

Modules::BaseModule* UnifiedClassContainer::getClassModule(const std::string& className) const {
    const ClassInfo& cls = findOrThrow(className, "Class not found");
    return cls.module;
}

const UnifiedClassContainer::PropertyInfo* UnifiedClassContainer::findProperty(
    const std::string& className, const std::string& propertyName) const {
    if (!hasClass(className)) {
        return nullptr;
    }
    
    // Start with the current class
    const ClassInfo& cls = classes_.at(className);
    
    // Check properties in the current class
    auto propIt = findPropertyInClass(cls, propertyName);
    if (propIt != cls.properties.end()) {
        return &(*propIt);
    }
    
    // If not found, check parent classes recursively
    if (!cls.parentClass.empty()) {
        return findProperty(cls.parentClass, propertyName);
    }
    
    return nullptr;
}

const UnifiedClassContainer::MethodInfo* UnifiedClassContainer::findMethod(
    const std::string& className, const std::string& methodName) const {
    if (!hasClass(className)) {
        return nullptr;
    }
    
    // Start with the current class
    const ClassInfo& cls = classes_.at(className);
    
    // Check methods in the current class
    auto methodIt = findMethodInClass(cls, methodName);
    if (methodIt != cls.methods.end()) {
        return &(*methodIt);
    }
    
    // If not found, check parent classes recursively
    if (!cls.parentClass.empty()) {
        return findMethod(cls.parentClass, methodName);
    }
    
    return nullptr;
}

ValuePtr UnifiedClassContainer::callMethod(
    const std::string& className, const std::string& methodName, 
    const std::vector<ValuePtr>& args) const {
    const MethodInfo* method = findMethod(className, methodName);
    if (!method) {
        throw ClassException("Method not found in class: " + methodName);
    }
    
    if (!method->nativeImplementation) {
        throw ClassException("Method does not have a native implementation: " + methodName);
    }
    
    // Call the native implementation
    return method->nativeImplementation(args);
}

UnifiedClassContainer::ClassInfo& UnifiedClassContainer::findOrThrow(
    const std::string& className, const std::string& errorMsg) {
    auto it = classes_.find(className);
    if (it == classes_.end()) {
        throw ClassException(errorMsg + ": " + className);
    }
    return it->second;
}

const UnifiedClassContainer::ClassInfo& UnifiedClassContainer::findOrThrow(
    const std::string& className, const std::string& errorMsg) const {
    auto it = classes_.find(className);
    if (it == classes_.end()) {
        throw ClassException(errorMsg + ": " + className);
    }
    return it->second;
}

std::vector<UnifiedClassContainer::PropertyInfo>::const_iterator 
UnifiedClassContainer::findPropertyInClass(
    const ClassInfo& classInfo, const std::string& propertyName) const {
    return std::find_if(
        classInfo.properties.begin(), 
        classInfo.properties.end(),
        [&propertyName](const PropertyInfo& prop) { return prop.name == propertyName; }
    );
}

std::vector<UnifiedClassContainer::MethodInfo>::const_iterator 
UnifiedClassContainer::findMethodInClass(
    const ClassInfo& classInfo, const std::string& methodName) const {
    return std::find_if(
        classInfo.methods.begin(), 
        classInfo.methods.end(),
        [&methodName](const MethodInfo& method) { return method.name == methodName; }
    );
}

} // namespace Symbols
