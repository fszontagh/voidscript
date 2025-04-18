// PrintLnModule.hpp
#ifndef MODULES_PRINTLNMODULE_HPP
#define MODULES_PRINTLNMODULE_HPP

#include <iostream>

#include "BaseModule.hpp"
#include "ModuleManager.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief Module that provides a built-in print function.
 */
class PrintNlModule : public BaseModule {
  public:
    void registerModule() override {
        auto & mgr = ModuleManager::instance();
        mgr.registerFunction("printnl", [](const std::vector<Symbols::Value> & args) {
            for (const auto & v : args) {
                std::cout << Symbols::Value::to_string(v);
            }
            std::cout << "\n";
            return Symbols::Value();
        });
    }
};

}  // namespace Modules
#endif  // MODULES_PrintLnModule_HPP
