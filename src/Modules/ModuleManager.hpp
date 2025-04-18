// ModuleManager.hpp
#ifndef MODULES_MODULEMANAGER_HPP
#define MODULES_MODULEMANAGER_HPP

#include <memory>
#include <vector>
#include "BaseModule.hpp"
#include <functional>
#include <unordered_map>
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief Manager for registering and invoking modules.
 */
class ModuleManager {
  public:
    /**
     * @brief Get singleton instance of ModuleManager.
     */
    static ModuleManager &instance() {
        static ModuleManager mgr;
        return mgr;
    }

    /**
     * @brief Add a module to the manager.
     * @param module Unique pointer to a BaseModule.
     */
    void addModule(std::unique_ptr<BaseModule> module) {
        modules_.push_back(std::move(module));
    }

    /**
     * @brief Invoke all registered modules to register their symbols.
     */
    void registerAll() {
        for (const auto &module : modules_) {
            module->registerModule();
        }
    }

  private:
    ModuleManager() = default;
    std::vector<std::unique_ptr<BaseModule>> modules_;
    // Built-in function callbacks: name -> function
    std::unordered_map<std::string,
        std::function<Symbols::Value(const std::vector<Symbols::Value>&)>> callbacks_;
  public:
    /**
     * @brief Register a built-in function callback.
     * @param name Name of the function.
     * @param cb Callable taking argument values and returning a Value.
     */
    void registerFunction(const std::string &name,
                          std::function<Symbols::Value(const std::vector<Symbols::Value>&)> cb) {
        callbacks_[name] = std::move(cb);
    }

    /**
     * @brief Check if a built-in function is registered.
     */
    bool hasFunction(const std::string &name) const {
        return callbacks_.find(name) != callbacks_.end();
    }

    /**
     * @brief Call a built-in function callback.
     */
    Symbols::Value callFunction(const std::string &name,
                                const std::vector<Symbols::Value> &args) const {
        auto it = callbacks_.find(name);
        if (it == callbacks_.end()) {
            throw std::runtime_error("Built-in function callback not found: " + name);
        }
        return it->second(args);
    }
};

} // namespace Modules
#endif // MODULES_MODULEMANAGER_HPP