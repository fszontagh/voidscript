// FormatModule implementation: String formatting using fmt library
#include "FormatModule.hpp"

#include <fmt/args.h>

#include "Modules/UnifiedModuleManager.hpp"

// Register module functions
void Modules::FormatModule::registerModule() {
    std::vector<FunctParameterInfo> param_list = {
        { "format", Symbols::Variables::Type::STRING, "The string to format" },
        { "interpolate...", Symbols::Variables::Type::STRING, "Parameters to replace '{}' placeoholders", true, true },
    };
    REGISTER_FUNCTION(this->name(), Symbols::Variables::Type::NULL_TYPE, param_list,
                      "Formats and prints text using fmt library. First argument is format string, followed by values "
                      "to interpolate.",
                      [](FunctionArguments & args) -> Symbols::Value {
                          if (args.size() < 2) {
                              throw std::runtime_error("2 arguments required");
                          }
                          if (args.front().getType() != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("First parameter need to be string");
                          }

                          auto         _args  = args;
                          const auto & format = Symbols::Value::to_string(_args.front());
                          _args.erase(_args.begin());

                          fmt::dynamic_format_arg_store<fmt::format_context> store;
                          for (const auto & arg : _args) {
                              store.push_back(Symbols::Value::to_string(arg));
                          }
                          std::cout << fmt::vformat(format, store);
                          return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
                      });

    param_list = {
        { "format", Symbols::Variables::Type::STRING, "The string to format" },
        { "interpolate...", Symbols::Variables::Type::STRING, "Values to interpolate", true, true },
    };

    REGISTER_FUNCTION(this->name(), Symbols::Variables::Type::STRING, param_list,
                      "Formats and returns string using fmt library. First argument is format string, followed by "
                      "values to interpolate.",
                      [](FunctionArguments & args) -> Symbols::Value {
                          if (args.size() < 2) {
                              throw std::runtime_error("2 arguments required");
                          }
                          if (args.front().getType() != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("First parameter need to be string");
                          }

                          auto         _args  = args;
                          const auto & format = Symbols::Value::to_string(_args.front());
                          _args.erase(_args.begin());

                          fmt::dynamic_format_arg_store<fmt::format_context> store;
                          for (const auto & arg : _args) {
                              store.push_back(Symbols::Value::to_string(arg));
                          }
                          return Symbols::Value(fmt::vformat(format, store));
                      });
}
