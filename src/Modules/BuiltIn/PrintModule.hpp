// PrintModule.hpp
#ifndef MODULES_PRINTMODULE_HPP
#define MODULES_PRINTMODULE_HPP

#include <iostream>

#include "Modules/BaseModule.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief Module that provides a built-in print function.
 */
class PrintModule : public BaseModule {
  public:
    PrintModule() { setModuleName("Print"); }

    void registerModule() override {
        // Register 'print' function
        REGISTER_FUNCTION("print", Symbols::Variables::Type::NULL_TYPE, {},
            "Output any to the standard output",
            [](const FunctionArguments& args) -> Symbols::Value {
                for (const auto& v : args) {
                    std::string valStr = Symbols::Value::to_string(v);
                    std::cout << valStr;
                }
                std::cout << std::flush;
                return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
            });
        // Register 'printnl' function
        REGISTER_FUNCTION("printnl", Symbols::Variables::Type::NULL_TYPE, {},
            "Output any to the standard output ending with new line",
            [](const FunctionArguments& args) -> Symbols::Value {
                for (const auto& v : args) {
                    std::string valStr = Symbols::Value::to_string(v);
                    std::cout << valStr;
                }
                std::cout << "\n" << std::flush;
                return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
            });
    }
};

}  // namespace Modules
#endif  // MODULES_PRINTMODULE_HPP
