#include "Symbols/ClassRegistry.hpp"

namespace Symbols {

// Initialize static members
ClassRegistry* ClassRegistry::instance_ = nullptr;

ClassRegistry::ClassRegistry() 
    : classContainer_(std::make_unique<UnifiedClassContainer>()),
      classFactory_(std::make_unique<ClassFactory>(*classContainer_)) {
}

ClassRegistry::~ClassRegistry() = default;

UnifiedClassContainer& ClassRegistry::getClassContainer() {
    return *classContainer_;
}

ClassFactory& ClassRegistry::getClassFactory() {
    return *classFactory_;
}

UnifiedClassContainer::ClassInfo& ClassRegistry::registerClass(
    const std::string& className, Modules::BaseModule* module) {
    return classContainer_->registerClass(className, module);
}

UnifiedClassContainer::ClassInfo& ClassRegistry::registerClass(
    const std::string& className, const std::string& parentClassName, Modules::BaseModule* module) {
    return classContainer_->registerClass(className, parentClassName, module);
}

bool ClassRegistry::hasClass(const std::string& className) const {
    return classContainer_->hasClass(className);
}

ValuePtr ClassRegistry::createInstance(
    const std::string& className, const std::vector<ValuePtr>& constructorArgs) {
    return classFactory_->createInstance(className, constructorArgs);
}

ValuePtr ClassRegistry::getStaticProperty(
    const std::string& className, const std::string& propertyName) const {
    return classContainer_->getStaticProperty(className, propertyName);
}

void ClassRegistry::setStaticProperty(
    const std::string& className, const std::string& propertyName, const ValuePtr& value) {
    classContainer_->setStaticProperty(className, propertyName, value);
}

bool ClassRegistry::hasStaticProperty(
    const std::string& className, const std::string& propertyName) const {
    return classContainer_->hasStaticProperty(className, propertyName);
}

ValuePtr ClassRegistry::getInstanceProperty(
    const ValuePtr& instance, const std::string& propertyName) {
    return classFactory_->getProperty(instance, propertyName);
}

void ClassRegistry::setInstanceProperty(
    ValuePtr& instance, const std::string& propertyName, const ValuePtr& value) {
    classFactory_->setProperty(instance, propertyName, value);
}

ValuePtr ClassRegistry::callMethod(
    const ValuePtr& instance, const std::string& methodName, const std::vector<ValuePtr>& args) {
    return classFactory_->callMethod(instance, methodName, args);
}

bool ClassRegistry::isInstanceOf(const ValuePtr& instance, const std::string& className) {
    return classFactory_->isInstanceOf(instance, className);
}

ClassRegistry& ClassRegistry::instance() {
    if (!instance_) {
        instance_ = new ClassRegistry();
    }
    return *instance_;
}

} // namespace Symbols
