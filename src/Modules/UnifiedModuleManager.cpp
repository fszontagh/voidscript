// DEPRECATED: This file is part of the legacy class/module management system.
// Use Symbols::ClassRegistry and related classes instead.
//
// TRANSITION: This module now delegates class management operations to Symbols::ClassRegistry
// while maintaining backward compatibility. It should ONLY be used directly from modules
// (built-in and dynamic). All other code should use Symbols::ClassRegistry.

#include "Modules/UnifiedModuleManager.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>

#include "utils.h"
#include "Symbols/ClassRegistry.hpp" // Add ClassRegistry include

using namespace Modules;

// --- Singleton instance ---
UnifiedModuleManager & UnifiedModuleManager::instance() {
    static UnifiedModuleManager mgr;
    return mgr;
}

const std::vector<Modules::FunctParameterInfo> & UnifiedModuleManager::getMethodParameters(
    const std::string & className, const std::string & methodName) const {
    // First try to use our backward compatibility structure
    if (classes_.find(className) != classes_.end() &&
        classes_.at(className).info.methods.find(methodName) != classes_.at(className).info.methods.end()) {
        return classes_.at(className).info.methods.at(methodName).parameters;
    }
    
    // For new method storage, we need to store parameters separately or in documentation
    // For now, return empty parameters
    static std::vector<FunctParameterInfo> empty;
    return empty;
    
    // Note: ClassRegistry doesn't provide a direct way to get method parameters
    // in the same format, so we can't delegate this call
}

// --- Module management ---
void UnifiedModuleManager::addModule(std::unique_ptr<BaseModule> module) {
    modules_.push_back(std::move(module));
}

void UnifiedModuleManager::registerAll() {
    for (const auto & module : modules_) {
        currentModule_ = module.get();
        module->registerModule();
    }
    currentModule_ = nullptr;
}

void UnifiedModuleManager::loadPlugins(const std::string & directory) {
    if (!utils::exists(directory) || !utils::is_directory(directory)) {
        return;
    }
    for (const auto & entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
#ifndef _WIN32
        if (entry.path().extension() != ".so") {
            continue;
        }
#else
        if (entry.path().extension() != ".dll") {
            continue;
        }
#endif
        loadPlugin(entry.path().string());
    }
}

void UnifiedModuleManager::loadPlugin(const std::string & path) {
    void * handle;
#ifndef _WIN32
    handle = dlopen(path.c_str(), RTLD_NOW);
    if (!handle) {
        throw std::runtime_error("Failed to load plugin: " + std::string(dlerror()));
    }

    using PluginInitFunc = void (*)();
    dlerror();  // Clear any existing errors
    PluginInitFunc init        = reinterpret_cast<PluginInitFunc>(dlsym(handle, "plugin_init"));
    const char *   dlsym_error = dlerror();
    if (dlsym_error) {
        dlclose(handle);
        throw std::runtime_error("Plugin missing 'plugin_init' symbol: " + path + ": " + std::string(dlsym_error));
    }
    init();
#else
    handle = LoadLibraryA(path.c_str());
    if (!handle) {
        throw std::runtime_error("Failed to load plugin: " + path);
    }

    using PluginInitFunc = void (*)();
    PluginInitFunc init  = reinterpret_cast<PluginInitFunc>(GetProcAddress((HMODULE) handle, "plugin_init"));
    if (!init) {
        FreeLibrary((HMODULE) handle);
        throw std::runtime_error("Plugin missing 'plugin_init' symbol: " + path);
    }
    init();
#endif

    // Store the module pointer before moving it
    BaseModule * modulePtr = modules_.empty() ? nullptr : modules_.back().get();
    plugins_.push_back({ handle, path, modulePtr });
}

// --- Function registration ---
void UnifiedModuleManager::registerFunction(const std::string & name, CallbackFunction cb,
                                            const Symbols::Variables::Type & returnType) {
    const auto it = functions_.find(name);
    if (it != functions_.end()) {
        throw std::runtime_error("Function already registered: " + name);
    }
    functions_[name].callback   = std::make_shared<CallbackFunction>(std::move(cb));
    functions_[name].returnType = returnType;
    functions_[name].module     = currentModule_;
}

void UnifiedModuleManager::registerDoc(const std::string & /*ModeName*/, const FunctionDoc & doc) {
    functions_[doc.name].doc = doc;
}

bool UnifiedModuleManager::hasFunction(const std::string & name) const {
    return functions_.find(name) != functions_.end();
}

Symbols::ValuePtr UnifiedModuleManager::callFunction(const std::string & name, FunctionArguments & args) const {
    const auto & entry = findOrThrow(functions_, name, "Function not found");
    return (*entry.callback)(args);
}

Symbols::Variables::Type UnifiedModuleManager::getFunctionReturnType(const std::string & name) {
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        return Symbols::Variables::Type::NULL_TYPE;
    }
    return it->second.returnType;
}

Symbols::ValuePtr UnifiedModuleManager::getFunctionNullValue(const std::string & name) {
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        return Symbols::ValuePtr::null(Symbols::Variables::Type::NULL_TYPE);
    }
    return Symbols::ValuePtr::null(it->second.returnType);
}

// --- Method registration (separate from functions) ---
void UnifiedModuleManager::registerMethod(const std::string & className, const std::string & methodName, 
                                          CallbackFunction cb, const Symbols::Variables::Type & returnType) {
    const std::string qualifiedName = createQualifiedMethodName(className, methodName);
    // Allow overriding existing methods instead of throwing an error
    methods_[qualifiedName].callback   = std::make_shared<CallbackFunction>(std::move(cb));
    methods_[qualifiedName].returnType = returnType;
    methods_[qualifiedName].module     = currentModule_;
}

Symbols::ValuePtr UnifiedModuleManager::callMethod(const std::string & className, const std::string & methodName, 
                                                   FunctionArguments & args) const {
    const std::string qualifiedName = createQualifiedMethodName(className, methodName);
    const auto & entry = findOrThrow(methods_, qualifiedName, "Method not found");
    return (*entry.callback)(args);
}

Symbols::Variables::Type UnifiedModuleManager::getMethodReturnType(const std::string & className, const std::string & methodName) {
    const std::string qualifiedName = createQualifiedMethodName(className, methodName);
    auto it = methods_.find(qualifiedName);
    if (it == methods_.end()) {
        return Symbols::Variables::Type::NULL_TYPE;
    }
    return it->second.returnType;
}

std::vector<std::string> UnifiedModuleManager::getMethodNames(const std::string & className) const {
    std::vector<std::string> methodNames;
    const std::string prefix = className + "::";
    
    for (const auto & [qualifiedName, entry] : methods_) {
        if (qualifiedName.substr(0, prefix.length()) == prefix) {
            std::string methodName = qualifiedName.substr(prefix.length());
            methodNames.push_back(methodName);
        }
    }
    
    return methodNames;
}

// --- Class registration ---
bool UnifiedModuleManager::hasClass(const std::string & className) const {
    // Delegate to ClassRegistry
    return Symbols::ClassRegistry::instance().hasClass(className);
}

void UnifiedModuleManager::registerClass(const std::string & className, const std::string & scopeName) {
    if (className.empty()) {
        throw std::runtime_error("Class name cannot be empty");
    }
    if (scopeName.empty()) {
        throw std::runtime_error("Scope name cannot be empty");
    }

    // No need to register in ClassRegistry here - that's now handled in the REGISTER_CLASS macro
    
    // Check if we already have this class in our local structures
    auto it = classes_.find(className);
    if (it != classes_.end()) {
        // Update the module and scope if needed
        it->second.module = currentModule_;
        it->second.scope = scopeName;
    } else {
        // Store in our local structures for backward compatibility
        classes_[className].info = ClassInfo();
        classes_[className].module = currentModule_;
        classes_[className].scope = scopeName;
    }
}

ClassInfo & UnifiedModuleManager::getClassInfo(const std::string & className) {
    return findOrThrow(classes_, className, "Class not found").info;
}

void UnifiedModuleManager::addProperty(const std::string & className, const std::string & propertyName,
                                       Symbols::Variables::Type type, Parser::ParsedExpressionPtr defaultValueExpr) {
    // Ensure the class exists in our backward compatibility structure
    ClassInfo & cls = findOrThrow(classes_, className, "Class not found").info;
    cls.properties.push_back({ propertyName, type, std::move(defaultValueExpr) });
    
    // We don't add properties to ClassRegistry here as that should be handled by the class implementation
}

void UnifiedModuleManager::addMethod(const std::string & className, const std::string & methodName) {
    // Keep backward compatibility
    findOrThrow(classes_, className, "Class not found").info.methodNames.push_back(methodName);
}

void UnifiedModuleManager::addMethod(const std::string & className, const std::string & methodName,
                                     std::function<Symbols::ValuePtr(const std::vector<Symbols::ValuePtr> &)> cb,
                                     const Symbols::Variables::Type & returnType) {
    // Create a copy of the callback before registering
    auto callback = std::move(cb);
    // Register method using the new method storage instead of function storage
    this->registerMethod(className, methodName, callback, returnType);
    
    // Add to backward compatibility structure only if not already present
    auto & classEntry = findOrThrow(classes_, className, "Class not found");
    auto & methodNames = classEntry.info.methodNames;
    if (std::find(methodNames.begin(), methodNames.end(), methodName) == methodNames.end()) {
        methodNames.push_back(methodName);
    }
}

void UnifiedModuleManager::setConstructor(const std::string& className, const std::string& constructorName) {
    findOrThrow(classes_, className, "Class not found").info.constructorName = constructorName;
    // We don't need to update ClassRegistry as constructors are handled differently there
}

// --- Object property management ---
void UnifiedModuleManager::setObjectProperty(const std::string & className, const std::string & propertyName,
                       const Symbols::ValuePtr & value) {
    // Use ClassRegistry to set static property
    Symbols::ClassRegistry::instance().setStaticProperty(className, propertyName, value);
    
    // Also update our backward compatibility structure
    auto & classEntry = findOrThrow(classes_, className, "Class not found");
    classEntry.info.objectProperties[propertyName] = value;
}

Symbols::ValuePtr UnifiedModuleManager::getObjectProperty(const std::string & className, const std::string & propertyName) const {
    // Use ClassRegistry to get static property
    if (Symbols::ClassRegistry::instance().hasStaticProperty(className, propertyName)) {
        return Symbols::ClassRegistry::instance().getStaticProperty(className, propertyName);
    }
    
    // Fall back to our backward compatibility structure
    const auto & classEntry = findOrThrow(classes_, className, "Class not found");
    auto it = classEntry.info.objectProperties.find(propertyName);
    if (it == classEntry.info.objectProperties.end()) {
        throw std::runtime_error("Property not found: " + className + "." + propertyName);
    }
    return it->second;
}

void UnifiedModuleManager::deleteObjectProperty(const std::string & className, const std::string & propertyName) {
    // There's no direct method to delete static properties in ClassRegistry,
    // but we can set it to null
    Symbols::ClassRegistry::instance().setStaticProperty(className, propertyName, Symbols::ValuePtr::null());
    
    // Remove from our backward compatibility structure
    auto & classEntry = findOrThrow(classes_, className, "Class not found");
    classEntry.info.objectProperties.erase(propertyName);
}

bool UnifiedModuleManager::hasObjectProperty(const std::string & className, const std::string & propertyName) const {
    // Check in ClassRegistry first
    if (Symbols::ClassRegistry::instance().hasStaticProperty(className, propertyName)) {
        return true;
    }
    
    // Fall back to our backward compatibility structure
    const auto & classEntry = findOrThrow(classes_, className, "Class not found");
    return classEntry.info.objectProperties.find(propertyName) != classEntry.info.objectProperties.end();
}

void UnifiedModuleManager::clearObjectProperties(const std::string & className) {
    // We can't easily clear all static properties in ClassRegistry
    // So we'll need to iterate through our known ones
    auto & classEntry = findOrThrow(classes_, className, "Class not found");
    for (const auto& prop : classEntry.info.objectProperties) {
        Symbols::ClassRegistry::instance().setStaticProperty(className, prop.first, Symbols::ValuePtr::null());
    }
    
    // Clear our backward compatibility structure
    classEntry.info.objectProperties.clear();
}

// --- Destructor ---
UnifiedModuleManager::~UnifiedModuleManager() {
    plugins_.clear();
    classes_.clear();
    functions_.clear();
    modules_.clear();  // std::unique_ptr free them
    for (const auto & plugin : plugins_) {
#ifndef _WIN32
        dlclose(plugin.handle);
#else
        FreeLibrary((HMODULE) plugin.handle);
#endif
    }
}

const Modules::FunctionDoc & Modules::UnifiedModuleManager::getFunctionDoc(const std::string & funcName) const {
    //return findOrThrow(functions_, funcName, "Function not found").doc;
    const auto it = functions_.find(funcName);
    if (it == functions_.end()) {
        throw std::runtime_error("function not registered: " + funcName);
    }
    return it->second.doc;
};

std::vector<std::string> UnifiedModuleManager::getClassNames() const {
    // Delegate to ClassRegistry
    return Symbols::ClassRegistry::instance().getClassContainer().getClassNames();
}

bool UnifiedModuleManager::hasMethod(const std::string & className, const std::string & methodName) const {
    // First check the new separate method storage
    const std::string qualifiedName = createQualifiedMethodName(className, methodName);
    if (methods_.find(qualifiedName) != methods_.end()) {
        return true;
    }
    
    // Fall back to backward compatibility structure for existing methods
    auto it = classes_.find(className);
    if (it == classes_.end()) {
        return false;
    }
    
    for (const auto & mName : it->second.info.methodNames) {
        if (mName == methodName) {
            return true;
        }
    }
    
    return false;
}

bool UnifiedModuleManager::hasProperty(const std::string & className, const std::string & propertyName) const {
    // Use our backward compatibility structure
    auto it = classes_.find(className);
    if (it == classes_.end()) {
        return false;
    }
    
    for (const auto & prop : it->second.info.properties) {
        if (prop.name == propertyName) {
            return true;
        }
    }
    
    return false;
}

// --- Missing function implementations ---

BaseModule * UnifiedModuleManager::getCurrentModule() const {
    return currentModule_;
}

std::string UnifiedModuleManager::getCurrentModuleName() const {
    return currentModule_ ? currentModule_->name() : "";
}

std::vector<std::string> UnifiedModuleManager::getFunctionNamesForModule(BaseModule * module) const {
    std::vector<std::string> names;
    
    // Get function names from the functions_ map
    for (const auto & [functionName, entry] : functions_) {
        if (entry.module == module) {
            names.push_back(functionName);
        }
    }
    
    // Get method names from the methods_ map for this module
    for (const auto & [qualifiedMethodName, entry] : methods_) {
        if (entry.module == module) {
            names.push_back(qualifiedMethodName);
        }
    }
    
    return names;
}

BaseModule * UnifiedModuleManager::getClassModule(const std::string & className) const {
    // Check the new class storage first
    if (Symbols::ClassRegistry::instance().getClassContainer().hasClass(className)) {
        return Symbols::ClassRegistry::instance().getClassContainer().getClassModule(className);
    }
    
    // Fall back to backward compatibility structure
    auto it = classes_.find(className);
    if (it != classes_.end()) {
        return it->second.module;
    }
    
    return nullptr;
}

std::vector<BaseModule *> UnifiedModuleManager::getPluginModules() const {
    std::vector<BaseModule *> modules;
    for (const auto & plugin : plugins_) {
        modules.push_back(plugin.module);
    }
    return modules;
}

std::vector<std::string> UnifiedModuleManager::getPluginPaths() const {
    std::vector<std::string> paths;
    for (const auto & plugin : plugins_) {
        paths.push_back(plugin.path);
    }
    return paths;
}
