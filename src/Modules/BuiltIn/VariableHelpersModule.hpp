// VariableHelpersModule.hpp
#ifndef MODULES_VARIABLEHELPERSMODULE_HPP
#define MODULES_VARIABLEHELPERSMODULE_HPP

#include <stdexcept>
#include <string>

#include "Modules/BaseModule.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

/**
 * @brief Module providing helper functions for variables.
 * Currently supports:
 *   typeof($var)            -> returns string name of type
 *   typeof($var, "int")     -> returns bool if type matches
 *   isset($var)             -> returns bool if variable is set
 *   var_dump($var)          -> returns string with detailed variable information
 */
class VariableHelpersModule : public BaseModule {
  private:
    static std::string var_dump_recursive(const Symbols::ValuePtr & value, int indent_level, int max_depth = 20) {
        // Prevent infinite recursion
        if (indent_level > max_depth) {
            return std::string(indent_level * 2, ' ') + "...[max depth reached]\n";
        }

        std::string indent(indent_level * 2, ' ');
        std::string result;

        if (!value) {
            return indent + "NULL\n";
        }

        Symbols::Variables::Type type      = value->getType();
        std::string              type_name = Symbols::Variables::TypeToString(type);

        switch (type) {
            case Symbols::Variables::Type::BOOLEAN:
                result += indent + type_name + "(" + (value->get<bool>() ? "true" : "false") + ")\n";
                break;

            case Symbols::Variables::Type::INTEGER:
                result += indent + type_name + "(" + std::to_string(value->get<int>()) + ")\n";
                break;

            case Symbols::Variables::Type::DOUBLE:
                result += indent + type_name + "(" + std::to_string(value->get<double>()) + ")\n";
                break;

            case Symbols::Variables::Type::FLOAT:
                result += indent + type_name + "(" + std::to_string(value->get<float>()) + ")\n";
                break;

            case Symbols::Variables::Type::STRING:
                {
                    std::string str_val = value->get<std::string>();
                    result += indent + type_name + "(" + std::to_string(str_val.length()) + ") \"" + str_val + "\"\n";
                }
                break;

            case Symbols::Variables::Type::OBJECT:
            case Symbols::Variables::Type::CLASS:
                {
                    result += indent + type_name + " {\n";

                    // Try to get object map for iteration
                    try {
                        const auto & obj_map = value->get<Symbols::ObjectMap>();
                        if (!obj_map.empty()) {
                            // Check if this looks like an array (all numeric keys)
                            bool looks_like_array = true;
                            for (const auto & [key, val] : obj_map) {
                                // Skip metadata keys when checking for array-like structure
                                if (key.find("__") != 0) {
                                    try {
                                        std::stoi(key);
                                    } catch (...) {
                                        looks_like_array = false;
                                        break;
                                    }
                                }
                            }

                            // Display elements
                            int index = 0;
                            for (const auto & [key, val] : obj_map) {
                                if (looks_like_array && key.find("__") != 0) {
                                    result += indent + "  [" + std::to_string(index++) + "] => \n";
                                } else {
                                    result += indent + "  [\"" + key + "\"] => \n";
                                }
                                result += var_dump_recursive(val, indent_level + 2, max_depth);
                            }
                        } else {
                            result += indent + "  [empty]\n";
                        }
                    } catch (const std::exception & e) {
                        // If we can't get object map, just show the object
                        result += indent + "  [content not accessible: " + std::string(e.what()) + "]\n";
                    }

                    result += indent + "}\n";
                }
                break;

            case Symbols::Variables::Type::NULL_TYPE:
                result += indent + "NULL\n";
                break;

            case Symbols::Variables::Type::UNDEFINED_TYPE:
                result += indent + "UNDEFINED\n";
                break;

            default:
                result += indent + type_name + "(" + value->toString() + ")\n";
                break;
        }

        return result;
    }

  public:
    VariableHelpersModule() { setModuleName("VariableHelpers"); }

    void registerFunctions() override {
        std::vector<Symbols::FunctionParameterInfo> param_list = {
            { "string", Symbols::Variables::Type::STRING, "The string to calculate the length of", false, false },
            { "string", Symbols::Variables::Type::STRING, "The type to compare against",           true,  false }
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
        std::vector<Symbols::FunctionParameterInfo> isset_param_list = {
            { "variable", Symbols::Variables::Type::OBJECT, "The variable to check if it is set", false, false }
        };
        REGISTER_FUNCTION("isset", Symbols::Variables::Type::BOOLEAN, isset_param_list,
                          "Check if a variable is set (not null/undefined)",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw std::runtime_error("isset expects exactly 1 argument");
                              }
                              return args[0] != Symbols::Variables::Type::UNDEFINED_TYPE;
                          });

        REGISTER_FUNCTION("is_null", Symbols::Variables::Type::BOOLEAN, isset_param_list, "Check if a variable is null",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw std::runtime_error("isset expects exactly 1 argument");
                              }
                              if (args[0] == Symbols::Variables::Type::UNDEFINED_TYPE) {
                                  return true;
                              }
                              return args[0]->isNULL();
                          });

        std::vector<Symbols::FunctionParameterInfo> var_dump_param_list = {
            { "variable", Symbols::Variables::Type::OBJECT, "The variable to dump", false, false }
        };

        REGISTER_FUNCTION("var_dump", Symbols::Variables::Type::STRING, var_dump_param_list,
                          "Display detailed information about a variable (type, value, structure)",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw std::runtime_error("var_dump expects exactly 1 argument");
                              }

                              return var_dump_recursive(args[0], 0);
                          });
    }
};

}  // namespace Modules

#endif  // MODULES_VARIABLEHELPERSMODULE_HPP
