// ModuleManager.hpp
#ifndef MODULES_MODULEMANAGER_HPP
#define MODULES_MODULEMANAGER_HPP

#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "BaseModule.hpp"
#include "Symbols/Value.hpp"

#ifndef _WIN32
#    include <dlfcn.h>
#else
#    include <windows.h>
#endif

namespace Modules {
using CallbackFunction = std::function<Symbols::Value(const std::vector<Symbols::Value> &)>;

/**
 * @brief Manager for registering and invoking modules.
 */
class ModuleManager {
  public:
    /**
     * @brief Get singleton instance of ModuleManager.
     */
    static ModuleManager & instance() {
        static ModuleManager mgr;
        return mgr;
    }

    /**
     * @brief Add a module to the manager.
     * @param module Unique pointer to a BaseModule.
     */
    void addModule(std::unique_ptr<BaseModule> module) { modules_.push_back(std::move(module)); }

    /**
     * @brief Invoke all registered modules to register their symbols.
     */
    void registerAll() {
        for (const auto & module : modules_) {
            module->registerModule();
        }
    }

  private:
    ModuleManager() = default;
    std::vector<std::unique_ptr<BaseModule>>                                                            modules_;
    // Built-in function callbacks: name -> function
    std::unordered_map<std::string, std::function<Symbols::Value(const std::vector<Symbols::Value> &)>> callbacks_;
    // Plugin handles for dynamically loaded modules
    std::vector<void *>                                                                                 pluginHandles_;
  public:
    /**
     * @brief Register a built-in function callback.
     * @param name Name of the function.
     * @param cb Callable taking argument values and returning a Value.
     */
    void registerFunction(const std::string &                                                name,
                          std::function<Symbols::Value(const std::vector<Symbols::Value> &)> cb) {
        callbacks_[name] = std::move(cb);
    }

    /**
     * @brief Check if a built-in function is registered.
     */
    bool hasFunction(const std::string & name) const { return callbacks_.find(name) != callbacks_.end(); }

    /**
     * @brief Call a built-in function callback.
     */
    Symbols::Value callFunction(const std::string & name, const std::vector<Symbols::Value> & args) const {
        auto it = callbacks_.find(name);
        if (it == callbacks_.end()) {
            throw std::runtime_error("Built-in function callback not found: " + name);
        }
        return it->second(args);
    }

    /**
     * @brief Load all plugin modules from specified directory.
     * @param directory Path to directory containing plugin shared libraries.
     */
    void loadPlugins(const std::string & directory) {
        namespace fs = std::filesystem;
        if (!fs::exists(directory) || !fs::is_directory(directory)) {
            return;
        }
        // Recursively search for plugin shared libraries
        for (const auto & entry : fs::recursive_directory_iterator(directory)) {
            if (!entry.is_regular_file()) {
                continue;
            }
#ifdef _WIN32
            if (entry.path().extension() == ".dll") {
#else
            if (entry.path().extension() == ".so") {
#endif
                loadPlugin(entry.path().string());
            }
        }
    }

    /**
     * @brief Load a single plugin module from shared library.
     * @param path Filesystem path to the shared library.
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

#ifndef _WIN32
        dlerror();  // clear any existing error
        using PluginInitFunc    = void (*)();
        auto         initFunc   = reinterpret_cast<PluginInitFunc>(dlsym(handle, "plugin_init"));
        const char * dlsymError = dlerror();
        if (dlsymError) {
            dlclose(handle);
            pluginHandles_.pop_back();
            throw std::runtime_error("Cannot find symbol 'plugin_init' in " + path + ": " + dlsymError);
        }
        initFunc();
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
     * @brief Destructor unloads modules and plugin libraries in safe order.
     * Modules (and their callback functions) are destroyed before the libraries are unloaded,
     * ensuring that destructors (in plugin code) run while the libraries are still mapped.
     */
    ~ModuleManager() {
        // Destroy module instances (may call code in plugin libraries)
        modules_.clear();
        // Clear callback functions (may refer to plugin code)
        callbacks_.clear();
        // Unload all dynamically loaded plugin libraries
        for (auto handle : pluginHandles_) {
#ifndef _WIN32
            dlclose(handle);
#else
            FreeLibrary((HMODULE) handle);
#endif
        }
        // Clear handles
        pluginHandles_.clear();
    }
};

}  // namespace Modules
#endif  // MODULES_MODULEMANAGER_HPP
