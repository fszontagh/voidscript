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
        std::vector<FunctParameterInfo> params = {
            { "msg", Symbols::Variables::Type::STRING, "The error message to throw" }
        };

        REGISTER_FUNCTION("throw_error", Symbols::Variables::Type::NULL_TYPE, params,
                          "Throw a runtime error and display error message, abort the script",
                          &PrintModule::ThrowError);
        params = {
            { "msgs...", Symbols::Variables::Type::STRING, "The error message to display", false, true },
        };
        REGISTER_FUNCTION("error", Symbols::Variables::Type::NULL_TYPE, params,
                          "Output a simple error message with newline end", &PrintModule::Error);

        params = {
            { "msgs...", Symbols::Variables::Type::STRING, "The message / variable to display", false, true },
        };

        REGISTER_FUNCTION("printnl", Symbols::Variables::Type::NULL_TYPE, params,
                          "Output any to the standard output ending wint new line", &PrintModule::PrintNL);

        REGISTER_FUNCTION("print", Symbols::Variables::Type::NULL_TYPE, params, "Output any to the standard output",
                          &PrintModule::Print);
    }

    static Symbols::ValuePtr ThrowError(const FunctionArguments & args) {
        if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
            throw Exception("throw_error requires exactly one string argument");
        }
        std::string msg = args[0];
        throw Exception(msg);
        return Symbols::ValuePtr::null();
    }

    static Symbols::ValuePtr Error(const FunctionArguments & args) {
        for (const auto & v : args) {
            std::cerr << v.toString();  // we will print everything here we don't care about the type
        }
        std::cerr << "\n";
        return Symbols::ValuePtr::null();
    }

    static Symbols::ValuePtr PrintNL(const FunctionArguments & args) {
        for (const auto & v : args) {
            std::cout << v.toString();
        }
        std::cout << "\n" << std::flush;
        return Symbols::ValuePtr::null();
    }

    static Symbols::ValuePtr Print(const FunctionArguments & args) {
        for (const auto & v : args) {
            std::cout << v.toString();
        }
        return Symbols::ValuePtr::null();
    }
};

}  // namespace Modules
#endif  // MODULES_PRINTMODULE_HPP
