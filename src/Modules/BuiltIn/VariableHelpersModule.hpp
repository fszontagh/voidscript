// VariableHelpersModule.hpp
#ifndef MODULES_VARIABLEHELPERSMODULE_HPP
#define MODULES_VARIABLEHELPERSMODULE_HPP

#include <stdexcept>
#include <string>

#include "Modules/BaseModule.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"
#include "Symbols/RegistrationMacros.hpp"

namespace Modules {

/**
 * @brief Module providing helper functions for variables.
 * Currently supports:
 *   typeof($var)            -> returns string name of type
 *   typeof($var, "int")   -> returns bool if type matches
 */
class VariableHelpersModule : public BaseModule {
  public:
    VariableHelpersModule() { setModuleName("VariableHelpers"); }

    void registerFunctions() override {
        std::vector<Symbols::FunctionParameterInfo> param_list = {
            { "string", Symbols::Variables::Type::STRING, "The string to calculate the length of", false, false },
            { "string", Symbols::Variables::Type::STRING, "The type to compare against", true, false }
        };
        REGISTER_FUNCTION("typeof", Symbols::Variables::Type::STRING, param_list, "Get the type of a variable",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() == 1) {
                                  Symbols::Variables::Type t = args[0]->getType();
                                  //return Symbols::ValuePtr(Symbols::Variables::TypeToString(t));
                                  return Symbols::Variables::TypeToString(t);
                              }
                              if (args.size() == 2) {
                                  auto t = args[0]->getType();
                                  if (args[1] != Symbols::Variables::Type::STRING) {
                                      throw std::runtime_error("Second argument to typeof must be string");
                                  }
                                  // Compare against provided type name via mapping
                                  const std::string provided = args[1];
                                  auto              expected = Symbols::Variables::StringToType(provided);
                                  return (t == expected);
                              }
                              throw std::runtime_error("typeof expects 1 or 2 arguments");
                          });
    }
};

}  // namespace Modules

#endif  // MODULES_VARIABLEHELPERSMODULE_HPP
