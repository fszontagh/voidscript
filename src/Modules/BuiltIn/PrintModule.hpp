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
        std::cerr << "[Debug][PrintModule] Registering print functions..." << std::endl;
        std::vector<FunctParameterInfo> params = {
            { "msg", Symbols::Variables::Type::STRING, "The error message to throw" }
        };

        REGISTER_FUNCTION("throw_error", Symbols::Variables::Type::NULL_TYPE, params,
                          "Throw a runtime error and display error message, abort the script",
                          [](const FunctionArguments& args) -> Symbols::Value {
                              if (args.size() != 1 || args[0].getType() != Symbols::Variables::Type::STRING) {
                                  throw Exception("throw_error requires exactly one string argument");
                              }
                              std::string msg = args[0].get<std::string>();
                              throw Exception(msg);
                              return Symbols::Value();  // never reached
                          });

        params = {
            { "msgs...", Symbols::Variables::Type::STRING, "The error message to display", false, true },
        };
        REGISTER_FUNCTION("error", Symbols::Variables::Type::NULL_TYPE, params,
                          "Output a simple error message with newline end",
                          [](const FunctionArguments& args) -> Symbols::Value {
                              for (const auto& v : args) {
                                  std::cerr << Symbols::Value::to_string(v);
                              }
                              std::cerr << "\n";
                              return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
                          });

        params = {
            { "msgs...", Symbols::Variables::Type::STRING, "The message / variable to display", false, true },
        };

        REGISTER_FUNCTION("printnl", Symbols::Variables::Type::NULL_TYPE, params,
                          "Output any to the standard output ending wint new line",
                          [](const FunctionArguments& args) -> Symbols::Value {
                              std::cerr << "[Debug][PrintModule] printnl called with " << args.size() << " args" << std::endl;
                              for (const auto& v : args) {
                                  std::string valStr = Symbols::Value::to_string(v);
                                  std::cout << valStr;
                              }
                              std::cout << "\n" << std::flush;
                              return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
                          });

        REGISTER_FUNCTION("print", Symbols::Variables::Type::NULL_TYPE, params,
                          "Output any to the standard output",
                          [](const FunctionArguments& args) -> Symbols::Value {
                              std::cerr << "[Debug][PrintModule] print called with " << args.size() << " args" << std::endl;
                              for (const auto& v : args) {
                                  std::string valStr = Symbols::Value::to_string(v);
                                  std::cout << valStr;
                              }
                              std::cout << std::flush;
                              return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
                          });
        std::cerr << "[Debug][PrintModule] Print functions registered." << std::endl;
    }
};

}  // namespace Modules
#endif  // MODULES_PRINTMODULE_HPP
