// FormatModule implementation: String formatting using fmt library
#include "FormatModule.hpp"

#include <fmt/args.h>

#include "Symbols/RegistrationMacros.hpp"

// Register module functions
void Modules::FormatModule::registerFunctions() {
    std::vector<Symbols::FunctionParameterInfo> param_list = {
        { "format", Symbols::Variables::Type::STRING, "The string to format" },
        { "interpolate...", Symbols::Variables::Type::STRING, "Parameters to replace '{}' placeoholders", true, true },
    };
    REGISTER_FUNCTION("format_print", Symbols::Variables::Type::NULL_TYPE, param_list,
                      "Formats and prints text using fmt library. First argument is format string, followed by values "
                      "to interpolate.",
                      [](FunctionArguments & args) -> Symbols::ValuePtr {
                          if (args.size() < 2) {
                              throw std::runtime_error("2 arguments required");
                          }
                          if (args.front() != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("First parameter need to be string");
                          }

                          auto                _args  = args;
                          const std::string & format = args[0];
                          _args.erase(_args.begin());

                          fmt::dynamic_format_arg_store<fmt::format_context> store;
                          for (const auto & arg : _args) {
                              store.push_back(arg.toString());
                          }
                          std::cout << fmt::vformat(format, store);
                          return Symbols::ValuePtr::null();
                      });

    param_list = {
        { "format", Symbols::Variables::Type::STRING, "The string to format" },
        { "interpolate...", Symbols::Variables::Type::STRING, "Values to interpolate", true, true },
    };

    REGISTER_FUNCTION("format", Symbols::Variables::Type::STRING, param_list,
                      "Formats and returns string using fmt library. First argument is format string, followed by "
                      "values to interpolate.",
                      [](FunctionArguments & args) -> Symbols::ValuePtr {
                          if (args.size() < 2) {
                              throw std::runtime_error("2 arguments required");
                          }
                          if (args.front() != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("First parameter need to be string");
                          }

                          auto              _args  = args;
                          const std::string format = _args.front();
                          _args.erase(_args.begin());

                          fmt::dynamic_format_arg_store<fmt::format_context> store;
                          for (const auto & arg : _args) {
                              store.push_back(arg.toString());
                          }
                          return Symbols::ValuePtr(fmt::vformat(format, store));
                      });
}
