#ifndef MODULES_CONVERSIONMODULE_HPP
#define MODULES_CONVERSIONMODULE_HPP

#include <string>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <cmath>

#include "Modules/BaseModule.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/RegistrationMacros.hpp"

namespace Modules {

/**
 * @brief Module providing conversion functions between strings and numbers.
 */
class ConversionModule : public BaseModule {
  public:
    ConversionModule() { setModuleName("Conversion"); }

    void registerFunctions() override {
        // string_to_number - Convert string to number (double)
        std::vector<Symbols::FunctionParameterInfo> param_list = {
            { "string", Symbols::Variables::Type::STRING, "The string to convert to a number", false, false }
        };

        REGISTER_FUNCTION("string_to_number", Symbols::Variables::Type::DOUBLE, param_list,
                          "Convert a string to a number (double). Supports both integer and floating-point formats.",
                          [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
                                  throw Exception(name() + "::string_to_number expects one string argument");
                              }
                              
                              const std::string str = args[0]->get<std::string>();
                              
                              // Handle empty string
                              if (str.empty()) {
                                  throw Exception(name() + "::string_to_number cannot convert empty string to number");
                              }
                              
                              // Trim whitespace
                              std::string trimmed = str;
                              trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r\f\v"));
                              trimmed.erase(trimmed.find_last_not_of(" \t\n\r\f\v") + 1);
                              
                              if (trimmed.empty()) {
                                  throw Exception(name() + "::string_to_number cannot convert whitespace-only string to number");
                              }
                              
                              try {
                                  size_t pos;
                                  double result = std::stod(trimmed, &pos);
                                  
                                  // Check if the entire string was consumed
                                  if (pos != trimmed.length()) {
                                      throw Exception(name() + "::string_to_number invalid number format: '" + str + "'");
                                  }
                                  
                                  // Check for infinity and NaN
                                  if (std::isinf(result)) {
                                      throw Exception(name() + "::string_to_number result is infinity: '" + str + "'");
                                  }
                                  if (std::isnan(result)) {
                                      throw Exception(name() + "::string_to_number result is not a number: '" + str + "'");
                                  }
                                  
                                  return result;
                              } catch (const std::invalid_argument &) {
                                  throw Exception(name() + "::string_to_number invalid number format: '" + str + "'");
                              } catch (const std::out_of_range &) {
                                  throw Exception(name() + "::string_to_number number out of range: '" + str + "'");
                              }
                          });

        // number_to_string - Convert number to string
        param_list = {
            { "number", Symbols::Variables::Type::DOUBLE, "The number to convert to a string", false, false }
        };
        
        REGISTER_FUNCTION("number_to_string", Symbols::Variables::Type::STRING, param_list,
                          "Convert a number to its string representation",
                          [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::number_to_string expects one argument");
                              }
                              
                              // Handle different numeric types
                              switch (args[0]->getType()) {
                                  case Symbols::Variables::Type::INTEGER: {
                                      int value = args[0]->get<int>();
                                      return std::to_string(value);
                                  }
                                  case Symbols::Variables::Type::DOUBLE: {
                                      double value = args[0]->get<double>();
                                      
                                      // Check for special values
                                      if (std::isinf(value)) {
                                          return value > 0 ? "inf" : "-inf";
                                      }
                                      if (std::isnan(value)) {
                                          return "nan";
                                      }
                                      
                                      // Use stringstream for proper formatting
                                      std::ostringstream oss;
                                      oss << value;
                                      return oss.str();
                                  }
                                  case Symbols::Variables::Type::FLOAT: {
                                      float value = args[0]->get<float>();
                                      
                                      // Check for special values
                                      if (std::isinf(value)) {
                                          return value > 0 ? "inf" : "-inf";
                                      }
                                      if (std::isnan(value)) {
                                          return "nan";
                                      }
                                      
                                      // Use stringstream for proper formatting
                                      std::ostringstream oss;
                                      oss << value;
                                      return oss.str();
                                  }
                                  default:
                                      throw Exception(name() + "::number_to_string expects a numeric argument (integer, float, or double)");
                              }
                          });
    }
};

}  // namespace Modules

#endif  // MODULES_CONVERSIONMODULE_HPP