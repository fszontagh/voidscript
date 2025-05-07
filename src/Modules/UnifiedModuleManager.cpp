#include "Modules/UnifiedModuleManager.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>

using namespace Modules;

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
#else
    handle = LoadLibraryA(path.c_str());
    if (!handle) {
        throw std::runtime_error("Failed to load plugin: " + path);
    }
#endif

    pluginHandles_.push_back(handle);

    using PluginInitFunc = void (*)();
#ifndef _WIN32
    dlerror();  // Clear any existing errors
    PluginInitFunc init        = reinterpret_cast<PluginInitFunc>(dlsym(handle, "plugin_init"));
    const char *   dlsym_error = dlerror();
    if (dlsym_error) {
        dlclose(handle);
        pluginHandles_.pop_back();
        throw std::runtime_error("Plugin missing 'plugin_init' symbol: " + path + ": " + std::string(dlsym_error));
    }
    init();
#else
    PluginInitFunc init = reinterpret_cast<PluginInitFunc>(GetProcAddress((HMODULE) handle, "plugin_init"));
    if (!init) {
        FreeLibrary((HMODULE) handle);
        pluginHandles_.pop_back();
        throw std::runtime_error("Plugin missing 'plugin_init' symbol: " + path);
    }
    init();
#endif

    pluginPaths_.push_back(path);
    if (!modules_.empty()) {
        pluginModules_.push_back(modules_.back().get());
    }
}

void UnifiedModuleManager::registerFunction(const std::string & name, CallbackFunction cb,
                                            const Symbols::Variables::Type & returnType) {
    functions_[name].callback   = cb;
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
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        throw std::runtime_error("Function not found: " + name);
    }
    return it->second.callback(args);
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

bool UnifiedModuleManager::hasClass(const std::string & className) const {
    return classes_.find(className) != classes_.end();
}

void UnifiedModuleManager::registerClass(const std::string & className) {
    classes_[className].info   = ClassInfo();
    classes_[className].module = currentModule_;
}

ClassInfo & UnifiedModuleManager::getClassInfo(const std::string & className) {
    return classes_.at(className).info;
}

void UnifiedModuleManager::addProperty(const std::string & className, const std::string & propertyName,
                                       Symbols::Variables::Type type, Parser::ParsedExpressionPtr defaultValueExpr) {
    ClassInfo & cls = classes_.at(className).info;
    cls.properties.push_back({ propertyName, type, std::move(defaultValueExpr) });
}

void UnifiedModuleManager::addMethod(const std::string & className, const std::string & methodName) {
    classes_.at(className).info.methodNames.push_back(methodName);
}

void UnifiedModuleManager::addMethod(const std::string & className, const std::string & methodName,
                                     std::function<Symbols::Value(const std::vector<Symbols::Value> &)> cb,
                                     const Symbols::Variables::Type &                                   returnType) {
    this->registerFunction(methodName, std::move(cb), returnType);
    classes_.at(className).info.methodNames.push_back(methodName);
}

bool UnifiedModuleManager::hasProperty(const std::string & className, const std::string & propertyName) const {
    const auto & props = classes_.at(className).info.properties;
    for (const auto & prop : props) {
        if (prop.name == propertyName) {
            return true;
        }
    }
    return false;
}

bool UnifiedModuleManager::hasMethod(const std::string & className, const std::string & methodName) const {
    const auto & methods = classes_.at(className).info.methodNames;
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
    return pluginPaths_;
}

std::vector<BaseModule *> UnifiedModuleManager::getPluginModules() const {
    return pluginModules_;
}

BaseModule * UnifiedModuleManager::getCurrentModule() const {
    return currentModule_;
}

std::string UnifiedModuleManager::getCurrentModuleName() const {
    return currentModule_ ? currentModule_->name() : "";
}

void UnifiedModuleManager::generateMarkdownDocs(const std::string & outputDir) const {
    std::filesystem::create_directories(outputDir);

    std::unordered_map<std::string, std::vector<std::string>> moduleFunctions;
    std::unordered_map<std::string, std::vector<std::string>> moduleClasses;

    for (const auto & [name, entry] : functions_) {
        if (entry.module) {
            std::cout << "Registering function '" << name << "' for module '" << entry.module->name() << "'\n";
            moduleFunctions[entry.module->name()].push_back(name);
        }
    }

    for (const auto & [name, entry] : classes_) {
        if (entry.module) {
            moduleClasses[entry.module->name()].push_back(name);
        }
    }

    for (const auto & [moduleName, functionNames] : moduleFunctions) {
        std::string filename = outputDir;
        filename += "/";
        filename += moduleName;
        filename += ".md";
        std::cout << "Generating documentation for module '" << moduleName << "'...\n";

        std::ofstream file(filename);
        if (!file.is_open()) {
            continue;
        }

        file << "# Module: " << moduleName << "\n\n";

        for (const auto & name : functionNames) {
            auto it = functions_.find(name);
            if (it == functions_.end()) {
                continue;
            }

            const auto & entry = it->second;
            const auto & doc   = entry.doc;

            file << "## Function: " << name << "\n";
            file << "Return Type: " << Symbols::Variables::TypeToString(entry.returnType) << "\n\n";

            if (!doc.description.empty()) {
                file << "Description: " << doc.description << "\n\n";
            }

            file << "Parameters:\n";
            for (const auto & param : doc.parameterList) {
                file << "- `" << param.name << "`: " << Symbols::Variables::TypeToString(param.type) << " - "
                     << param.description << "\n";
            }

            file << "\n";
        }

        for (const auto & className : moduleClasses[moduleName]) {
            auto classIt = classes_.find(className);
            if (classIt == classes_.end()) {
                continue;
            }

            const auto & classEntry = classIt->second;
            const auto & module     = classEntry.module;

            file << "## Class: " << className << "\n";

            for (const auto & prop : classEntry.info.properties) {
                file << "### Property: " << prop.name << "\n";
                file << "Type: " << Symbols::Variables::TypeToString(prop.type) << "\n\n";
            }

            for (const auto & methodName : classEntry.info.methodNames) {
                auto methodIt = functions_.find(methodName);
                if (methodIt == functions_.end()) {
                    continue;
                }

                const auto & methodEntry = methodIt->second;
                const auto & methodDoc   = methodEntry.doc;

                file << "### Method: " << methodName << "\n";
                file << "Return Type: " << Symbols::Variables::TypeToString(methodEntry.returnType) << "\n\n";

                if (!methodDoc.description.empty()) {
                    file << "Description: " << methodDoc.description << "\n\n";
                }

                file << "Parameters:\n";
                for (const auto & param : methodDoc.parameterList) {
                    file << "- `" << param.name << "`: " << Symbols::Variables::TypeToString(param.type) << " - "
                         << param.description << "\n";
                }

                file << "\n";
            }
        }

        file.close();
    }
}

UnifiedModuleManager::~UnifiedModuleManager() {
    // Cleanup plugin handles
    for (void * handle : pluginHandles_) {
#ifndef _WIN32
        dlclose(handle);
#else
        FreeLibrary((HMODULE) handle);
#endif
    }

    functions_.clear();
    pluginHandles_.clear();
    pluginPaths_.clear();


    pluginModules_.clear();


    classes_.clear();

    modules_.clear(); // std::unique_ptr free them
}
