#include "Symbols/ClassContainer.hpp"
#include "Symbols/ClassContainerAdapter.hpp"

static_assert(sizeof(Symbols::ClassContainer::ClassInfo) > 0, "ClassInfo is not visible after include!");

namespace Symbols {

// Initialize static members
ClassContainer* ClassContainer::instance_ = nullptr;
const std::vector<functionParameterType> ClassContainer::empty_parameters_ = {};

ClassContainer* ClassContainer::instance() {
    if (!instance_) {
        // Use the adapter to the new class system instead of the original implementation
        instance_ = new ClassContainerAdapter();
    }
    return instance_;
}

Symbols::ClassContainer::ClassInfo& Symbols::ClassContainer::registerClass(const std::string& className, Modules::BaseModule* module) {
    if (hasClass(className)) {
        throw Exception("Class already registered: " + className);
    }

    classes_[className] = ClassContainer::ClassInfo{
        .name = className,
        .parentClass = "",
        .properties = {},
        .methods = {},
        .objectProperties = {},
        .module = module
    };

    return classes_[className];
}

Symbols::ClassContainer::ClassInfo& Symbols::ClassContainer::registerClass(const std::string& className, 
                                       const std::string& parentClassName, 
                                       Modules::BaseModule* module) {
    if (hasClass(className)) {
        throw Exception("Class already registered: " + className);
    }

    if (!parentClassName.empty() && !hasClass(parentClassName)) {
        throw Exception("Parent class not registered: " + parentClassName);
    }

    classes_[className] = ClassContainer::ClassInfo{
        .name = className,
        .parentClass = parentClassName,
        .properties = {},
        .methods = {},
        .objectProperties = {},
        .module = module
    };

    return classes_[className];
}

bool Symbols::ClassContainer::hasClass(const std::string& className) const {
    return classes_.find(className) != classes_.end();
}

Symbols::ClassContainer::ClassInfo& Symbols::ClassContainer::getClassInfo(const std::string& className) {
    return findOrThrow(className, "Class not found");
}

const Symbols::ClassContainer::ClassInfo& Symbols::ClassContainer::getClassInfo(const std::string& className) const {
    return findOrThrow(className, "Class not found");
}

void Symbols::ClassContainer::addProperty(const std::string& className, 
                               const std::string& propertyName, 
                               Variables::Type type, 
                               bool isPrivate,
                               Parser::ParsedExpressionPtr defaultValueExpr) {
    ClassContainer::ClassInfo& cls = findOrThrow(className, "Class not found");
    
    // Check if property already exists
    for (const auto& prop : cls.properties) {
        if (prop.name == propertyName) {
            throw Exception("Property already exists in class: " + propertyName);
        }
    }

    cls.properties.push_back(ClassContainer::PropertyInfo{
        .name = propertyName,
        .type = type,
        .defaultValueExpr = std::move(defaultValueExpr),
        .isPrivate = isPrivate
    });
}

void Symbols::ClassContainer::addMethod(const std::string& className, 
                             const std::string& methodName,
                             Variables::Type returnType,
                             const std::vector<functionParameterType>& parameters,
                             bool isPrivate) {
    ClassContainer::ClassInfo& cls = findOrThrow(className, "Class not found");
    
    // Check if method already exists
    for (const auto& method : cls.methods) {
        if (method.name == methodName) {
            throw Exception("Method already exists in class: " + methodName);
        }
    }

    std::string qualifiedName = className + "::" + methodName;

    cls.methods.push_back(ClassContainer::MethodInfo{
        .name = methodName,
        .qualifiedName = qualifiedName,
        .returnType = returnType,
        .parameters = parameters,
        .isPrivate = isPrivate
    });
}

bool ClassContainer::hasProperty(const std::string& className, const std::string& propertyName) const {
    return findProperty(className, propertyName) != nullptr;
}

bool ClassContainer::hasMethod(const std::string& className, const std::string& methodName) const {
    return findMethod(className, methodName) != nullptr;
}

std::vector<std::string> ClassContainer::getClassNames() const {
    std::vector<std::string> names;
    names.reserve(classes_.size());
    
    for (const auto& pair : classes_) {
        names.push_back(pair.first);
    }
    
    return names;
}

Variables::Type Symbols::ClassContainer::getPropertyType(const std::string& className, 
                                             const std::string& propertyName) const {
    const ClassContainer::PropertyInfo* prop = findProperty(className, propertyName);
    if (!prop) {
        throw Exception("Property not found in class: " + propertyName);
    }
    
    return prop->type;
}

Variables::Type Symbols::ClassContainer::getMethodReturnType(const std::string& className, 
                                                 const std::string& methodName) const {
    const ClassContainer::MethodInfo* method = findMethod(className, methodName);
    if (!method) {
        throw Exception("Method not found in class: " + methodName);
    }
    
    return method->returnType;
}

const std::vector<functionParameterType>& Symbols::ClassContainer::getMethodParameters(
    const std::string& className, const std::string& methodName) const {
    const ClassContainer::MethodInfo* method = findMethod(className, methodName);
    if (!method) {
        throw Exception("Method not found in class: " + methodName);
    }
    
    return method->parameters;
}

void Symbols::ClassContainer::setObjectProperty(const std::string& className, 
                                     const std::string& propertyName, 
                                     const ValuePtr& value) {
    ClassContainer::ClassInfo& cls = findOrThrow(className, "Class not found");
    cls.objectProperties[propertyName] = value;
}

ValuePtr Symbols::ClassContainer::getObjectProperty(const std::string& className, 
                                         const std::string& propertyName) const {
    const ClassContainer::ClassInfo& cls = findOrThrow(className, "Class not found");
    
    auto it = cls.objectProperties.find(propertyName);
    if (it == cls.objectProperties.end()) {
        throw Exception("Object property not found: " + propertyName);
    }
    
    return it->second;
}

bool Symbols::ClassContainer::hasObjectProperty(const std::string& className, 
                                     const std::string& propertyName) const {
    if (!hasClass(className)) {
        return false;
    }
    
    const ClassContainer::ClassInfo& cls = classes_.at(className);
    return cls.objectProperties.find(propertyName) != cls.objectProperties.end();
}

void Symbols::ClassContainer::deleteObjectProperty(const std::string& className, 
                                        const std::string& propertyName) {
    ClassContainer::ClassInfo& cls = findOrThrow(className, "Class not found");
    cls.objectProperties.erase(propertyName);
}

void Symbols::ClassContainer::clearObjectProperties(const std::string& className) {
    ClassContainer::ClassInfo& cls = findOrThrow(className, "Class not found");
    cls.objectProperties.clear();
}

Modules::BaseModule* Symbols::ClassContainer::getClassModule(const std::string& className) const {
    const ClassContainer::ClassInfo& cls = findOrThrow(className, "Class not found");
    return cls.module;
}

Symbols::ClassContainer::ClassInfo& Symbols::ClassContainer::findOrThrow(const std::string& className, const std::string& errorMsg) {
    auto it = classes_.find(className);
    if (it == classes_.end()) {
        throw Exception(errorMsg + ": " + className);
    }
    return it->second;
}

const Symbols::ClassContainer::ClassInfo& Symbols::ClassContainer::findOrThrow(const std::string& className, 
                                           const std::string& errorMsg) const {
    auto it = classes_.find(className);
    if (it == classes_.end()) {
        throw Exception(errorMsg + ": " + className);
    }
    return it->second;
}

const Symbols::ClassContainer::PropertyInfo* Symbols::ClassContainer::findProperty(
    const std::string& className, const std::string& propertyName) const {
    
    if (!hasClass(className)) {
        return nullptr;
    }
    
    // Check current class
    const ClassContainer::ClassInfo& cls = classes_.at(className);
    for (const auto& prop : cls.properties) {
        if (prop.name == propertyName) {
            return &prop;
        }
    }
    
    // Check parent class if exists
    if (!cls.parentClass.empty()) {
        return findProperty(cls.parentClass, propertyName);
    }
    
    return nullptr;
}

const Symbols::ClassContainer::MethodInfo* Symbols::ClassContainer::findMethod(
    const std::string& className, const std::string& methodName) const {
    
    if (!hasClass(className)) {
        return nullptr;
    }
    
    // Check current class
    const ClassContainer::ClassInfo& cls = classes_.at(className);
    for (const auto& method : cls.methods) {
        if (method.name == methodName) {
            return &method;
        }
    }
    
    // Check parent class if exists
    if (!cls.parentClass.empty()) {
        return findMethod(cls.parentClass, methodName);
    }
    
    return nullptr;
}

} // namespace Symbols
