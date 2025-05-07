#ifndef MODULES_STRINGMODULE_HPP
#define MODULES_STRINGMODULE_HPP

#include <string>

#include "Modules/BaseModule.hpp"
#include "Modules/IModuleContext.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

class StringModule : public BaseModule {
  public:
    StringModule() { setModuleName("String"); }

    void registerModule(IModuleContext & context) override {
        // string_length
        std::vector<FunctParameterInfo> param_list = {
            {"string", Symbols::Variables::Type::STRING}
        };
        REGISTER_MODULE_FUNCTION(context, this->name(), "string_length", Symbols::Variables::Type::INTEGER, param_list,
                                 "Calculate the length of a string",
                                 [this](FunctionArguments & args) -> Symbols::Value {
                                     if (args.size() != 1 || args[0].getType() != Symbols::Variables::Type::STRING) {
                                         throw Exception(name() + "::string_length expects one string argument");
                                     }
                                     return Symbols::Value(static_cast<int>(args[0].get<std::string>().size()));
                                 });

        // string_replace
        param_list = {
            { "string", Symbols::Variables::Type::STRING },
            { "from",   Symbols::Variables::Type::STRING },
            { "to",     Symbols::Variables::Type::STRING }
        };
        REGISTER_MODULE_FUNCTION(context, this->name(), "string_replace", Symbols::Variables::Type::STRING, param_list,
                                 "Replace part of a string with another string",
                                 [this](FunctionArguments & args) -> Symbols::Value {
                                     if (args.size() < 3) {
                                         throw Exception(name() + "::string_replace expects at least 3 arguments");
                                     }
                                     std::string str  = args[0].get<std::string>();
                                     std::string from = args[1].get<std::string>();
                                     std::string to   = args[2].get<std::string>();
                                     size_t      pos  = str.find(from);
                                     if (pos != std::string::npos) {
                                         str.replace(pos, from.length(), to);
                                     }
                                     return Symbols::Value(str);
                                 });

        // string_substr
        param_list = {
            { "string", Symbols::Variables::Type::STRING  },
            { "start",  Symbols::Variables::Type::INTEGER },
            { "length", Symbols::Variables::Type::INTEGER }
        };
        REGISTER_MODULE_FUNCTION(context, this->name(), "string_substr", Symbols::Variables::Type::STRING, param_list,
                                 "Extract a substring from a string",
                                 [this](FunctionArguments & args) -> Symbols::Value {
                                     if (args.size() != 3) {
                                         throw Exception(name() + "::string_substr expects 3 arguments");
                                     }
                                     std::string str    = args[0].get<std::string>();
                                     int         start  = args[1].get<int>();
                                     int         length = args[2].get<int>();
                                     return Symbols::Value(str.substr(start, length));
                                 });
    }
};

}  // namespace Modules

#endif  // MODULES_STRINGMODULE_HPP
