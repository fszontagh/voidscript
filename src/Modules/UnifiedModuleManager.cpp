#include "Modules/UnifiedModuleManager.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <iostream>

using namespace Modules;

// --- Singleton instance ---
UnifiedModuleManager & UnifiedModuleManager::instance() {
    static UnifiedModuleManager mgr;
    return mgr;
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
    if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
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
    PluginInitFunc init = reinterpret_cast<PluginInitFunc>(dlsym(handle, "plugin_init"));
    const char * dlsym_error = dlerror();
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
    PluginInitFunc init = reinterpret_cast<PluginInitFunc>(GetProcAddress((HMODULE) handle, "plugin_init"));
    if (!init) {
        FreeLibrary((HMODULE) handle);
        throw std::runtime_error("Plugin missing 'plugin_init' symbol: " + path);
    }
    init();
#endif

    // Store the module pointer before moving it
    BaseModule* modulePtr = modules_.empty() ? nullptr : modules_.back().get();
    plugins_.push_back({handle, path, modulePtr});
}

// --- Function registration ---
void UnifiedModuleManager::registerFunction(const std::string & name, CallbackFunction cb,
                                            const Symbols::Variables::Type & returnType) {
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

Symbols::Value UnifiedModuleManager::callFunction(const std::string & name, FunctionArguments & args) const {
    const auto& entry = findOrThrow(functions_, name, "Function not found");
    return (*entry.callback)(args);
}

Symbols::Variables::Type UnifiedModuleManager::getFunctionReturnType(const std::string & name) {
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        return Symbols::Variables::Type::NULL_TYPE;
    }
    return it->second.returnType;
}

Symbols::Value UnifiedModuleManager::getFunctionNullValue(const std::string & name) {
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
    }
    return Symbols::Value::makeNull(it->second.returnType);
}

// --- Class registration ---
bool UnifiedModuleManager::hasClass(const std::string & className) const {
    return classes_.find(className) != classes_.end();
}

void UnifiedModuleManager::registerClass(const std::string & className) {
    classes_[className].info   = ClassInfo();
    classes_[className].module = currentModule_;
}

ClassInfo & UnifiedModuleManager::getClassInfo(const std::string & className) {
    return findOrThrow(classes_, className, "Class not found").info;
}

void UnifiedModuleManager::addProperty(const std::string & className, const std::string & propertyName,
                                       Symbols::Variables::Type type, Parser::ParsedExpressionPtr defaultValueExpr) {
    ClassInfo & cls = findOrThrow(classes_, className, "Class not found").info;
    cls.properties.push_back({ propertyName, type, std::move(defaultValueExpr) });
}

void UnifiedModuleManager::addMethod(const std::string & className, const std::string & methodName) {
    findOrThrow(classes_, className, "Class not found").info.methodNames.push_back(methodName);
}

void UnifiedModuleManager::addMethod(const std::string & className, const std::string & methodName,
                                     std::function<Symbols::Value(const std::vector<Symbols::Value> &)> cb,
                                     const Symbols::Variables::Type & returnType) {
    // Create a copy of the callback before registering
    auto callback = cb;
    this->registerFunction(methodName, callback, returnType);
    findOrThrow(classes_, className, "Class not found").info.methodNames.push_back(methodName);
}

bool UnifiedModuleManager::hasProperty(const std::string & className, const std::string & propertyName) const {
    const auto & props = findOrThrow(classes_, className, "Class not found").info.properties;
    return std::any_of(props.begin(), props.end(), 
                      [&propertyName](const auto& prop) { return prop.name == propertyName; });
}

bool UnifiedModuleManager::hasMethod(const std::string & className, const std::string & methodName) const {
    const auto & methods = findOrThrow(classes_, className, "Class not found").info.methodNames;
    return std::find(methods.begin(), methods.end(), methodName) != methods.end();
}

std::vector<std::string> UnifiedModuleManager::getClassNames() const {
    std::vector<std::string> names;
    names.reserve(classes_.size());
    for (const auto & pair : classes_) {
        names.push_back(pair.first);
    }
    return names;
}

// --- Object property management ---
void UnifiedModuleManager::setObjectProperty(const std::string& className, const std::string& propertyName, const Symbols::Value& value) {
    auto& classEntry = findOrThrow(classes_, className, "Class not found");
    classEntry.info.objectProperties[propertyName] = value;
}

Symbols::Value UnifiedModuleManager::getObjectProperty(const std::string& className, const std::string& propertyName) const {
    const auto& classEntry = findOrThrow(classes_, className, "Class not found");
    auto it = classEntry.info.objectProperties.find(propertyName);
    if (it == classEntry.info.objectProperties.end()) {
        return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
    }
    return it->second;
}

void UnifiedModuleManager::deleteObjectProperty(const std::string& className, const std::string& propertyName) {
    auto& classEntry = findOrThrow(classes_, className, "Class not found");
    classEntry.info.objectProperties.erase(propertyName);
}

bool UnifiedModuleManager::hasObjectProperty(const std::string& className, const std::string& propertyName) const {
    const auto& classEntry = findOrThrow(classes_, className, "Class not found");
    return classEntry.info.objectProperties.find(propertyName) != classEntry.info.objectProperties.end();
}

void UnifiedModuleManager::clearObjectProperties(const std::string& className) {
    auto& classEntry = findOrThrow(classes_, className, "Class not found");
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
    for (const auto& plugin : plugins_) {
        paths.push_back(plugin.path);
    }
    return paths;
}

std::vector<BaseModule *> UnifiedModuleManager::getPluginModules() const {
    std::vector<BaseModule *> modules;
    modules.reserve(plugins_.size());
    for (const auto& plugin : plugins_) {
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
void UnifiedModuleManager::writeDoc(std::ofstream& file, 
                                  const std::string& name,
                                  const RegistryEntry& entry,
                                  const std::string& prefix,
                                  const Symbols::Variables::Type& returnType) const {
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
    std::filesystem::create_directories(outputDir);

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
    auto it = functions_.find(clsname);
    if (it != functions_.end()) {
        return it->second.module;
    }
    return nullptr;
}

// --- Destructor ---
UnifiedModuleManager::~UnifiedModuleManager() {

    plugins_.clear();
    classes_.clear();
    functions_.clear();
    modules_.clear();  // std::unique_ptr free them
    for (const auto& plugin : plugins_) {
#ifndef _WIN32
        dlclose(plugin.handle);
#else
        FreeLibrary((HMODULE)plugin.handle);
#endif
    }    
}
