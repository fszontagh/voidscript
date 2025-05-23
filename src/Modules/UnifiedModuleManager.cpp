// DEPRECATED: This file is part of the legacy class/module management system.
// Use Symbols::ClassRegistry and related classes instead.
//
// TRANSITION: This module now delegates class management operations to Symbols::ClassRegistry
// while maintaining backward compatibility. It should ONLY be used directly from modules
// (built-in and dynamic). All other code should use Symbols::ClassRegistry.

#include "Modules/UnifiedModuleManager.hpp"

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
    
    // If not found, return empty parameters
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
    this->registerFunction(methodName, callback, returnType);
    findOrThrow(classes_, className, "Class not found").info.methodNames.push_back(methodName);
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

// --- Utility functions ---
std::vector<std::string> UnifiedModuleManager::getFunctionNamesForModule(BaseModule * module) const {
    std::vector<std::string> names;
    for (const auto & pair : functions_) {
        if (pair.second.module == module) {
            names.push_back(pair.first);
        }
    }
    return names;
}

std::vector<std::string> UnifiedModuleManager::getPluginPaths() const {
    std::vector<std::string> paths;
    paths.reserve(plugins_.size());
    for (const auto & plugin : plugins_) {
        paths.push_back(plugin.path);
    }
    return paths;
}

std::vector<BaseModule *> UnifiedModuleManager::getPluginModules() const {
    std::vector<BaseModule *> modules;
    modules.reserve(plugins_.size());
    for (const auto & plugin : plugins_) {
        modules.push_back(plugin.module);
    }
    return modules;
}

BaseModule * UnifiedModuleManager::getCurrentModule() const {
    return currentModule_;
}

std::string UnifiedModuleManager::getCurrentModuleName() const {
    return currentModule_ ? currentModule_->name() : "";
}

// --- Documentation helpers ---
void UnifiedModuleManager::writeDoc(std::ofstream & file, const std::string & name, const RegistryEntry & entry,
                                    const std::string & prefix, const Symbols::Variables::Type & returnType) const {
    file << prefix << name << "\n";
    file << "Return Type: " << Symbols::Variables::TypeToString(returnType) << "\n\n";

    if (!entry.doc.description.empty()) {
        file << "Description: " << entry.doc.description << "\n\n";
    }

    file << "Parameters:\n";
    for (const auto & param : entry.doc.parameterList) {
        file << "- `" << param.name << "`: " << Symbols::Variables::TypeToString(param.type) << " - "
             << param.description << "\n";
    }
    file << "\n";
}

void UnifiedModuleManager::generateMarkdownDocs(const std::string & outputDir) const {
    utils::create_directories(outputDir);

    std::unordered_map<std::string, std::vector<std::string>> moduleFunctions;
    std::unordered_map<std::string, std::vector<std::string>> moduleClasses;

    // Collect functions and classes by module
    for (const auto & [name, entry] : functions_) {
        if (entry.module) {
            std::string moduleName = normalizeModuleName(entry.module->name());
            moduleFunctions[moduleName].push_back(name);
        }
    }

    for (const auto & [name, entry] : classes_) {
        if (entry.module) {
            std::string moduleName = normalizeModuleName(entry.module->name());
            moduleClasses[moduleName].push_back(name);
        }
    }

    // Generate documentation for each module
    for (const auto & [moduleName, functionNames] : moduleFunctions) {
        std::string filename = outputDir + "/" + moduleName + ".md";

        std::ofstream file(filename);
        if (!file.is_open()) {
            continue;
        }

        file << "# Module: " << moduleName << "\n\n";

        // Write function documentation
        for (const auto & name : functionNames) {
            auto it = functions_.find(name);
            if (it == functions_.end()) {
                continue;
            }
            writeDoc(file, name, it->second, "## Function: ", it->second.returnType);
        }

        // Write class documentation
        for (const auto & className : moduleClasses[moduleName]) {
            auto classIt = classes_.find(className);
            if (classIt == classes_.end()) {
                continue;
            }

            const auto & classEntry = classIt->second;
            file << "## Class: " << className << "\n";

            // Write properties
            for (const auto & prop : classEntry.info.properties) {
                file << "### Property: " << prop.name << "\n";
                file << "Type: " << Symbols::Variables::TypeToString(prop.type) << "\n\n";
            }

            // Write methods
            for (const auto & methodName : classEntry.info.methodNames) {
                auto methodIt = functions_.find(methodName);
                if (methodIt == functions_.end()) {
                    continue;
                }
                writeDoc(file, methodName, methodIt->second, "### Method: ", methodIt->second.returnType);
            }
        }

        file.close();
    }
}

// --- Macro compatibility: getClassModule ---
BaseModule * UnifiedModuleManager::getClassModule(const std::string & clsname) const {
    // First try to get the module from ClassRegistry via its container
    try {
        auto& container = Symbols::ClassRegistry::instance().getClassContainer();
        return container.getClassModule(clsname);
    } catch (const std::exception&) {
        // If that fails, fall back to our legacy behavior
        auto it = functions_.find(clsname);
        if (it != functions_.end()) {
            return it->second.module;
        }
        return nullptr;
    }
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
    // Use our backward compatibility structure
    auto it = classes_.find(className);
    if (it == classes_.end()) {
        return false;
    }
    
    for (const auto & mName : it->second.info.methodNames) {
        if (mName == methodName) {
            return true;
        }
    }
    
    // ClassRegistry doesn't provide a direct way to check for methods
    // so we can't delegate this call
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
