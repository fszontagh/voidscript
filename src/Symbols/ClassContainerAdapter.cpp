#include "Symbols/ClassContainerAdapter.hpp"

namespace Symbols {

// Initialize static members
ClassContainerAdapter* ClassContainerAdapter::instance_ = nullptr;

ClassContainerAdapter* ClassContainerAdapter::instance() {
    if (!instance_) {
        instance_ = new ClassContainerAdapter();
    }
    return instance_;
}

ClassContainerAdapter::ClassContainerAdapter() {
    // Initialize the adapter
}

ClassContainer::ClassInfo& ClassContainerAdapter::registerClass(
    const std::string& className, Modules::BaseModule* module) {
    // Register the class in the new system
    ClassRegistry::instance().registerClass(className, module);
    
    // Create a proxy class info object
    classInfoProxies_[className] = ClassInfo{
        .name = className,
        .parentClass = "",
        .properties = {},
        .methods = {},
        .objectProperties = {},
        .module = module
    };
    
    return classInfoProxies_[className];
}

ClassContainer::ClassInfo& ClassContainerAdapter::registerClass(
    const std::string& className, const std::string& parentClassName, Modules::BaseModule* module) {
    // Register the class in the new system
    ClassRegistry::instance().registerClass(className, parentClassName, module);
    
    // Create a proxy class info object
    classInfoProxies_[className] = ClassInfo{
        .name = className,
        .parentClass = parentClassName,
        .properties = {},
        .methods = {},
        .objectProperties = {},
        .module = module
    };
    
    return classInfoProxies_[className];
}

bool ClassContainerAdapter::hasClass(const std::string& className) const {
    return ClassRegistry::instance().hasClass(className);
}

ClassContainer::ClassInfo& ClassContainerAdapter::getClassInfo(const std::string& className) {
    if (!hasClass(className)) {
        throw Exception("Class not found: " + className);
    }
    
    // Return the proxy class info
    return classInfoProxies_[className];
}

const ClassContainer::ClassInfo& ClassContainerAdapter::getClassInfo(const std::string& className) const {
    if (!hasClass(className)) {
        throw Exception("Class not found: " + className);
    }
    
    // Return the proxy class info
    return classInfoProxies_.at(className);
}

void ClassContainerAdapter::addProperty(
    const std::string& className, const std::string& propertyName, 
    Variables::Type type, bool isPrivate, Parser::ParsedExpressionPtr defaultValueExpr) {
    // Add the property to the new system
    ClassRegistry::instance().getClassContainer().addProperty(
        className, propertyName, type, isPrivate, std::move(defaultValueExpr));
    
    // Update the proxy class info
    if (classInfoProxies_.find(className) != classInfoProxies_.end()) {
        classInfoProxies_[className].properties.push_back(PropertyInfo{
            .name = propertyName,
            .type = type,
            .defaultValueExpr = nullptr,  // We can't move it twice
            .isPrivate = isPrivate
        });
    }
}

void ClassContainerAdapter::addMethod(
    const std::string& className, const std::string& methodName,
    Variables::Type returnType, const std::vector<functionParameterType>& parameters, bool isPrivate) {
    // Convert parameters to the new format
    std::vector<UnifiedClassContainer::ParameterInfo> newParams;
    for (const auto& param : parameters) {
        newParams.push_back(convertFunctionParameter(param));
    }
    
    // Add the method to the new system
    ClassRegistry::instance().getClassContainer().addMethod(
        className, methodName, returnType, newParams, isPrivate);
    
    // Update the proxy class info
    if (classInfoProxies_.find(className) != classInfoProxies_.end()) {
        std::string qualifiedName = className + "::" + methodName;
        
        classInfoProxies_[className].methods.push_back(MethodInfo{
            .name = methodName,
            .qualifiedName = qualifiedName,
            .returnType = returnType,
            .parameters = parameters,
            .isPrivate = isPrivate
        });
    }
    
    // Store method parameters for later retrieval
    methodParameters_[className + "::" + methodName] = parameters;
}

bool ClassContainerAdapter::hasProperty(const std::string& className, const std::string& propertyName) const {
    return ClassRegistry::instance().getClassContainer().hasProperty(className, propertyName);
}

bool ClassContainerAdapter::hasMethod(const std::string& className, const std::string& methodName) const {
    return ClassRegistry::instance().getClassContainer().hasMethod(className, methodName);
}

std::vector<std::string> ClassContainerAdapter::getClassNames() const {
    return ClassRegistry::instance().getClassContainer().getClassNames();
}

Variables::Type ClassContainerAdapter::getPropertyType(
    const std::string& className, const std::string& propertyName) const {
    return ClassRegistry::instance().getClassContainer().getPropertyType(className, propertyName);
}

Variables::Type ClassContainerAdapter::getMethodReturnType(
    const std::string& className, const std::string& methodName) const {
    return ClassRegistry::instance().getClassContainer().getMethodReturnType(className, methodName);
}

const std::vector<functionParameterType>& ClassContainerAdapter::getMethodParameters(
    const std::string& className, const std::string& methodName) const {
    std::string key = className + "::" + methodName;
    
    if (methodParameters_.find(key) != methodParameters_.end()) {
        return methodParameters_.at(key);
    }
    
    static const std::vector<functionParameterType> empty;
    return empty;
}

void ClassContainerAdapter::setObjectProperty(
    const std::string& className, const std::string& propertyName, const ValuePtr& value) {
    ClassRegistry::instance().setStaticProperty(className, propertyName, value);
    
    // Update the proxy class info
    if (classInfoProxies_.find(className) != classInfoProxies_.end()) {
        classInfoProxies_[className].objectProperties[propertyName] = value;
    }
}

ValuePtr ClassContainerAdapter::getObjectProperty(
    const std::string& className, const std::string& propertyName) const {
    return ClassRegistry::instance().getStaticProperty(className, propertyName);
}

bool ClassContainerAdapter::hasObjectProperty(
    const std::string& className, const std::string& propertyName) const {
    return ClassRegistry::instance().hasStaticProperty(className, propertyName);
}

void ClassContainerAdapter::deleteObjectProperty(
    const std::string& className, const std::string& propertyName) {
    // Delete from the new system
    ClassRegistry::instance().getClassContainer().deleteStaticProperty(className, propertyName);
    
    // Update the proxy class info
    if (classInfoProxies_.find(className) != classInfoProxies_.end()) {
        classInfoProxies_[className].objectProperties.erase(propertyName);
    }
}

void ClassContainerAdapter::clearObjectProperties(const std::string& className) {
    // Clear in the new system
    ClassRegistry::instance().getClassContainer().clearStaticProperties(className);
    
    // Update the proxy class info
    if (classInfoProxies_.find(className) != classInfoProxies_.end()) {
        classInfoProxies_[className].objectProperties.clear();
    }
}

Modules::BaseModule* ClassContainerAdapter::getClassModule(const std::string& className) const {
    return ClassRegistry::instance().getClassContainer().getClassModule(className);
}

functionParameterType ClassContainerAdapter::convertParameterInfo(
    const UnifiedClassContainer::ParameterInfo& info) const {
    return functionParameterType{
        .name = info.name,
        .type = info.type
    };
}

UnifiedClassContainer::ParameterInfo ClassContainerAdapter::convertFunctionParameter(
    const functionParameterType& param) const {
    return UnifiedClassContainer::ParameterInfo{
        .name = param.name,
        .type = param.type,
        .description = "",
        .optional = false,
        .interpolate = false
    };
}

} // namespace Symbols
