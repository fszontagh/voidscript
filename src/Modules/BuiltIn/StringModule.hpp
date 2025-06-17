#ifndef MODULES_STRINGMODULE_HPP
#define MODULES_STRINGMODULE_HPP

#include <string>

#include "Modules/BaseModule.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/RegistrationMacros.hpp"

namespace Modules {

class StringModule : public BaseModule {
  public:
    StringModule() {
        setModuleName("String");
        setDescription("Provides string manipulation and processing functions including length calculation, replacement, and substring extraction");
    }

    void registerFunctions() override {
        // string_length
        std::vector<Symbols::FunctionParameterInfo> param_list = {
            { "string", Symbols::Variables::Type::STRING, "The string to calculate the length of", false, false }
        };

        REGISTER_FUNCTION("string_length", Symbols::Variables::Type::INTEGER, param_list,
                          "Calculate the length of a string", [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw Exception(name() + "::string_length expects one string argument");
                              }
                              const std::string str  = args[0];
                              size_t            size = str.size();
                              return static_cast<int>(size);
                          });

        // string_replace
        param_list = {
            { "string", Symbols::Variables::Type::STRING, "The string in which to replace", false, false },
            { "from",   Symbols::Variables::Type::STRING, "The string to replace from", false, false },
            { "to",     Symbols::Variables::Type::STRING, "The string to replace to", false, false }
        };
        REGISTER_FUNCTION("string_replace", Symbols::Variables::Type::STRING, param_list,
                          "Replace part of a string with another string",
                          [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() < 3) {
                                  throw Exception(name() + "::string_replace expects at least 3 arguments");
                              }
                              std::string str  = args[0];
                              std::string from = args[1];
                              std::string to   = args[2];
                              size_t      pos  = str.find(from);
                              if (pos != std::string::npos) {
                                  str.replace(pos, from.length(), to);
                              }
                              return str;
                          });

        // string_substr
        param_list = {
            { "string", Symbols::Variables::Type::STRING,  "The string to extract a substring from" },
            { "start",  Symbols::Variables::Type::INTEGER, "The start index of the substring"       },
            { "length", Symbols::Variables::Type::INTEGER, "The length of the substring"            }
        };
        REGISTER_FUNCTION("string_substr", Symbols::Variables::Type::STRING, param_list,
                          "Extract a substring from a string", [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 3) {
                                  throw Exception(name() + "::string_substr expects 3 arguments");
                              }
                              std::string str    = args[0];
                              int         start  = args[1];
                              int         length = args[2];
                              return str.substr(start, length);
                          });
    }
};

}  // namespace Modules

#endif  // MODULES_STRINGMODULE_HPP
