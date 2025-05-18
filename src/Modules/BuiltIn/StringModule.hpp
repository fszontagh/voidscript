#ifndef MODULES_STRINGMODULE_HPP
#define MODULES_STRINGMODULE_HPP

#include <string>

#include "Modules/BaseModule.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

class StringModule : public BaseModule {
  public:
    StringModule() { setModuleName("String"); }

    void registerModule() override {
        // string_length
        std::vector<FunctParameterInfo> param_list = {
            { "string", Symbols::Variables::Type::STRING, "The string to calculate the length of" }
        };

        REGISTER_FUNCTION("string_length", Symbols::Variables::Type::INTEGER, param_list,
                          "Calculate the length of a string",
                          [this](FunctionArguments & args) -> Symbols::Value::ValuePtr {
                              if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
                                  throw Exception(name() + "::string_length expects one string argument");
                              }
                              const std::string str  = *args[0];
                              size_t            size = str.size();
                              return Symbols::Value::create(size);
                          });

        // string_replace
        param_list = {
            { "string", Symbols::Variables::Type::STRING, "The string in which to replace" },
            { "from",   Symbols::Variables::Type::STRING, "The string to replace from"     },
            { "to",     Symbols::Variables::Type::STRING, "The string to replace to"       }
        };
        REGISTER_FUNCTION("string_replace", Symbols::Variables::Type::STRING, param_list,
                          "Replace part of a string with another string",
                          [this](FunctionArguments & args) -> Symbols::Value::ValuePtr {
                              if (args.size() < 3) {
                                  throw Exception(name() + "::string_replace expects at least 3 arguments");
                              }
                              std::string str  = *args[0];
                              std::string from = *args[1];
                              std::string to   = *args[2];
                              size_t      pos  = str.find(from);
                              if (pos != std::string::npos) {
                                  str.replace(pos, from.length(), to);
                              }
                              return Symbols::Value::create(str);
                          });

        // string_substr
        param_list = {
            { "string", Symbols::Variables::Type::STRING,  "The string to extract a substring from" },
            { "start",  Symbols::Variables::Type::INTEGER, "The start index of the substring"       },
            { "length", Symbols::Variables::Type::INTEGER, "The length of the substring"            }
        };
        REGISTER_FUNCTION("string_substr", Symbols::Variables::Type::STRING, param_list,
                          "Extract a substring from a string",
                          [this](FunctionArguments & args) -> Symbols::Value::ValuePtr {
                              if (args.size() != 3) {
                                  throw Exception(name() + "::string_substr expects 3 arguments");
                              }
                              std::string str    = *args[0];
                              int         start  = *args[1];
                              int         length = *args[2];
                              return Symbols::Value::create(str.substr(start, length));
                          });
    }
};

}  // namespace Modules

#endif  // MODULES_STRINGMODULE_HPP
