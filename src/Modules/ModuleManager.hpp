#ifndef MODULES_MODULEMANAGER_HPP
#define MODULES_MODULEMANAGER_HPP

#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef _WIN32
#    include <dlfcn.h>
#else
#    include <windows.h>
#endif

#include "BaseModule.hpp"
#include "IModuleContext.hpp"
#include "Symbols/Value.hpp"

namespace Modules {
using FunctionArguments = const std::vector<Symbols::Value>;
using CallbackFunction  = std::function<Symbols::Value(FunctionArguments &)>;

/**
 * @brief Singleton manager for loading and registering modules and functions.
 */
class ModuleManager : public IModuleContext {

  public:
    /**
     * @brief Retrieve singleton instance.
     */
    [[deprecated("Use UnifiedModuleManager instead")]] static ModuleManager & instance() {
        static ModuleManager mgr;
        return mgr;
    }

    /**
     * @brief Add a statically defined module.
     */
    void addModule(std::unique_ptr<BaseModule> module) { modules_.emplace_back(std::move(module)); }

    /**
     * @brief Register all symbols from modules.
     * Tracks the current module for introspection purposes.
     */
    void registerAll() {
        for (const auto & module : modules_) {
            currentModule_ = module.get();
            module->registerModule();
        }
        currentModule_ = nullptr;
    }

    /**
     * @brief Register a function (e.g., from a module) to the symbol table.
     */
    [[deprecated("Use instead UnifiedModuleManager")]] void registerFunction(
        const std::string & name, CallbackFunction cb,
        const Symbols::Variables::Type & returnType = Symbols::Variables::Type::NULL_TYPE) override {
        callbacks_[name]             = std::move(cb);
        callbacks_return_type_[name] = returnType;
        functionModuleMap_[name]     = currentModule_;
    }

    /**
     * @brief Register documentation for a function.
     * @param modName The name of the module
     * @param doc see @struct FunctionDoc
     */
    void registerDoc(const std::string & modName, const FunctionDoc & doc) override { this->docuMap_[modName] = doc; }

    /**
     * @brief Check if a function is registered.
     */
    bool hasFunction(const std::string & name) const { return callbacks_.find(name) != callbacks_.end(); }

    /**
     * @brief Call a registered function by name.
     */
    Symbols::Value callFunction(const std::string & name, FunctionArguments & args) const {
        auto it = callbacks_.find(name);
        if (it == callbacks_.end()) {
            throw std::runtime_error("Built-in function callback not found: " + name);
        }
        return it->second(args);
    }

    /**
     * @brief Get return type of a registered function.
     */
    Symbols::Variables::Type getFunctionReturnType(const std::string & name) {
        auto it = callbacks_return_type_.find(name);
        return it != callbacks_return_type_.end() ? it->second : Symbols::Variables::Type::NULL_TYPE;
    }

    /**
     * @brief Get default null value for the return type of a function.
     */
    Symbols::Value getFunctionNullValue(const std::string & name) {
        return Symbols::Value::makeNull(getFunctionReturnType(name));
    }

    /**
     * @brief Get all function names registered by a specific module.
     */
    std::vector<std::string> getFunctionNamesForModule(BaseModule * module) const {
        std::vector<std::string> result;
        for (const auto & [name, mod] : functionModuleMap_) {
            if (mod == module) {
                result.push_back(name);
            }
        }
        return result;
    }

    /**
     * @brief Get list of loaded plugin paths.
     */
    std::vector<std::string> getPluginPaths() const { return pluginPaths_; }

    /**
     * @brief Get loaded plugin module instances.
     */
    std::vector<BaseModule *> getPluginModules() const { return pluginModules_; }

    /**
     * @brief Get currently registering module.
     */
    BaseModule * getCurrentModule() const { return currentModule_; }

    /**
     * @brief Get the name of the currently registering module.
     */
    std::string getCurrentModuleName() const override { return currentModule_ ? currentModule_->name() : ""; }

    /**
     * @brief Load plugins from directory.
     */
    void loadPlugins(const std::string & directory) {
        namespace fs = std::filesystem;
        if (!fs::exists(directory) || !fs::is_directory(directory)) {
            return;
        }

        for (const auto & entry : fs::recursive_directory_iterator(directory)) {
            if (!entry.is_regular_file()) {
                continue;
            }

#ifdef _WIN32
            if (entry.path().extension() == ".dll")
#else
            if (entry.path().extension() == ".so")
#endif
            {
                loadPlugin(entry.path().string());
            }
        }
    }

    /**
     * @brief Load a single plugin from shared library.
     */
    void loadPlugin(const std::string & path) {
#ifndef _WIN32
        void * handle = dlopen(path.c_str(), RTLD_NOW);
        if (!handle) {
            throw std::runtime_error("Failed to load module: " + path + ": " + dlerror());
        }
#else
        HMODULE handle = LoadLibraryA(path.c_str());
        if (!handle) {
            throw std::runtime_error("Failed to load module: " + path);
        }
#endif
        pluginHandles_.push_back(handle);
        size_t beforeCount = modules_.size();

#ifndef _WIN32
        dlerror();  // Clear any existing errors
        using PluginInitFunc    = void (*)();
        auto         initFunc   = reinterpret_cast<PluginInitFunc>(dlsym(handle, "plugin_init"));
        const char * dlsymError = dlerror();
        if (dlsymError) {
            dlclose(handle);
            pluginHandles_.pop_back();
            throw std::runtime_error("Cannot find symbol 'plugin_init' in " + path + ": " + dlsymError);
        }
        initFunc();
        size_t afterCount = modules_.size();
        for (size_t i = beforeCount; i < afterCount; ++i) {
            pluginModules_.push_back(modules_[i].get());
            pluginPaths_.push_back(path);
        }
#else
        using PluginInitFunc = void(__cdecl *)();
        auto initFunc        = reinterpret_cast<PluginInitFunc>(GetProcAddress(handle, "plugin_init"));
        if (!initFunc) {
            FreeLibrary(handle);
            pluginHandles_.pop_back();
            throw std::runtime_error("Cannot find symbol 'plugin_init' in " + path);
        }
        initFunc();
#endif
    }

    /**
     * @brief Destructor: safely cleans up modules and unloads plugins.
     */
    ~ModuleManager() {
        modules_.clear();
        callbacks_.clear();
        callbacks_return_type_.clear();

        for (auto handle : pluginHandles_) {
#ifndef _WIN32
            dlclose(handle);
#else
            FreeLibrary((HMODULE) handle);
#endif
        }

        pluginHandles_.clear();
    }

  private:
    ModuleManager() = default;

    std::vector<std::unique_ptr<BaseModule>>                  modules_;
    std::unordered_map<std::string, CallbackFunction>         callbacks_;
    std::unordered_map<std::string, Symbols::Variables::Type> callbacks_return_type_;
    std::vector<void *>                                       pluginHandles_;
    std::vector<BaseModule *>                                 pluginModules_;
    std::vector<std::string>                                  pluginPaths_;
    BaseModule *                                              currentModule_ = nullptr;
    std::unordered_map<std::string, BaseModule *>             functionModuleMap_;
    std::unordered_map<std::string, FunctionDoc>              docuMap_;
};

}  // namespace Modules

#endif  // MODULES_MODULEMANAGER_HPP
