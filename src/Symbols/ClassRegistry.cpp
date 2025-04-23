#include "Symbols/ClassRegistry.hpp"

#include <stdexcept>
#include <utility>

#include "Modules/ModuleManager.hpp"

namespace Symbols {

ClassRegistry & ClassRegistry::instance() {
    static ClassRegistry inst;
    return inst;
}

// Register a new property for a class
void ClassRegistry::addProperty(const std::string & className, const std::string & propertyName, Variables::Type type,
                                Parser::ParsedExpressionPtr defaultValueExpr) {
    ClassInfo &             info = getClassInfo(className);
    ClassInfo::PropertyInfo prop{ propertyName, type, std::move(defaultValueExpr) };
    info.properties.push_back(std::move(prop));
}

// Register a new method for a class
void ClassRegistry::addMethod(const std::string & className, const std::string & methodName) {
    ClassInfo & info = getClassInfo(className);
    info.methodNames.push_back(methodName);
    // Record module that registered this method
    {
        std::string key       = className + "::" + methodName;
        methodModuleMap_[key] = Modules::ModuleManager::instance().getCurrentModule();
    }
}

void ClassRegistry::addMethod(const std::string & className, const std::string & methodName,
                              std::function<Symbols::Value(const std::vector<Symbols::Value> &)> cb,
                              const Symbols::Variables::Type &                                   returnType) {
    ClassInfo & info = getClassInfo(className);
    info.methodNames.push_back(methodName);
    // Record module that registered this method
    {
        std::string key       = className + "::" + methodName;
        methodModuleMap_[key] = Modules::ModuleManager::instance().getCurrentModule();
    }

    auto & mgr = Modules::ModuleManager::instance();
    mgr.registerFunction(className + "::" + methodName, std::move(cb), returnType);
}

// Check if a property exists in the class
bool ClassRegistry::hasProperty(const std::string & className, const std::string & propertyName) const {
    auto it = classes_.find(className);
    if (it == classes_.end()) {
        return false;
    }
    const auto & props = it->second.properties;
    for (const auto & prop : props) {
        if (prop.name == propertyName) {
            return true;
        }
    }
    return false;
}

// Check if a method exists in the class
bool ClassRegistry::hasMethod(const std::string & className, const std::string & methodName) const {
    auto it = classes_.find(className);
    if (it == classes_.end()) {
        return false;
    }
    const auto & methods = it->second.methodNames;
    for (const auto & m : methods) {
        if (m == methodName) {
            return true;
        }
    }
    return false;
}

// Get all registered class names
std::vector<std::string> ClassRegistry::getClassNames() const {
    std::vector<std::string> names;
    names.reserve(classes_.size());
    for (const auto & pair : classes_) {
        names.push_back(pair.first);
    }
    return names;
}

bool ClassRegistry::hasClass(const std::string & className) const {
    return classes_.find(className) != classes_.end();
}

void ClassRegistry::registerClass(const std::string & className) {
    // Insert a new class, overwrite existing if present
    // Use operator[] to ensure existing entries are replaced rather than ignored
    classes_[className]        = ClassInfo();
    // Record module that registered this class
    classModuleMap_[className] = Modules::ModuleManager::instance().getCurrentModule();
}

ClassInfo & ClassRegistry::getClassInfo(const std::string & className) {
    auto it = classes_.find(className);
    if (it == classes_.end()) {
        throw std::runtime_error("Class not found: " + className);
    }
    return it->second;
}

}  // namespace Symbols

// --- Module lookup for classes and methods ---
namespace Symbols {
Modules::BaseModule * ClassRegistry::getClassModule(const std::string & className) const {
    auto it = classModuleMap_.find(className);
    return (it != classModuleMap_.end()) ? it->second : nullptr;
}

Modules::BaseModule * ClassRegistry::getMethodModule(const std::string & className,
                                                     const std::string & methodName) const {
    std::string key = className + "::" + methodName;
    auto        it  = methodModuleMap_.find(key);
    return (it != methodModuleMap_.end()) ? it->second : nullptr;
}
}  // namespace Symbols
