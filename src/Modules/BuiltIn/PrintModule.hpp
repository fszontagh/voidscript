// PrintModule.hpp
#ifndef MODULES_PRINTMODULE_HPP
#define MODULES_PRINTMODULE_HPP

#include <iostream>

#include "Modules/BaseModule.hpp"
#include "Modules/ModuleManager.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief Module that provides a built-in print function.
 */
class PrintModule : public BaseModule {
  public:
    void registerModule() override {
        auto & mgr = ModuleManager::instance();
        mgr.registerFunction("print", [](const std::vector<Symbols::Value> & args) {
            for (const auto & v : args) {
                std::cout << Symbols::Value::to_string(v);
            }
            return Symbols::Value();
        });
        mgr.registerFunction("printnl", [](const std::vector<Symbols::Value> & args) {
            for (const auto & v : args) {
                std::cout << Symbols::Value::to_string(v);
            }
            std::cout << "\n";
            return Symbols::Value();
        });
        mgr.registerFunction("error", [](const std::vector<Symbols::Value> & args) {
            for (const auto & v : args) {
                std::cerr << Symbols::Value::to_string(v);
            }
            std::cerr << "\n";
            return Symbols::Value();
        });
    }
};

}  // namespace Modules
#endif  // MODULES_PRINTMODULE_HPP
