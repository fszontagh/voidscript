// PrintModule.hpp
#ifndef MODULES_PRINTMODULE_HPP
#define MODULES_PRINTMODULE_HPP

#include <iostream>
#include <exception>

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
        mgr.registerFunction("print", [](FuncionArguments & args) {
            for (const auto & v : args) {
                std::cout << Symbols::Value::to_string(v);
            }
            return Symbols::Value();
        });
        mgr.registerFunction("printnl", [](FuncionArguments & args) {
            for (const auto & v : args) {
                 std::string valStr = Symbols::Value::to_string(v);
                 std::cout << valStr;
            }
            std::cout << "\n" << std::flush; 
            return Symbols::Value();
        });
        mgr.registerFunction("error", [](FuncionArguments & args) {
            for (const auto & v : args) {
                std::cerr << Symbols::Value::to_string(v);
            }
            std::cerr << "\n";
            return Symbols::Value();
        });
        // Built-in error thrower: throws a module exception with provided message
        mgr.registerFunction("throw_error", [](FuncionArguments & args) {
            if (args.size() != 1 || args[0].getType() != Symbols::Variables::Type::STRING) {
                throw Exception("throw_error requires exactly one string argument");
            }
            std::string msg = args[0].get<std::string>();
            throw Exception(msg);
            return Symbols::Value();  // never reached
        });
    }
};

}  // namespace Modules
#endif  // MODULES_PRINTMODULE_HPP
