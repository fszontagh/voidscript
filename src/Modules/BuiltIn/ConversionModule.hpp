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
    ConversionModule() {
        setModuleName("Conversion");
        setDescription("Provides data type conversion functions between strings, numbers, and other primitive types with robust error handling");
        setBuiltIn(true);
    }

    void registerFunctions() override {
        // string_to_number - Convert string to number (auto-detects type)
        std::vector<Symbols::FunctionParameterInfo> param_list = {
            { "string", Symbols::Variables::Type::STRING, "The string to convert to a number", false, false }
        };

        REGISTER_FUNCTION("string_to_number", Symbols::Variables::Type::DOUBLE, param_list,
                          "Convert a string to a number. Auto-detects whether the input is an integer, float, or double and returns the appropriate type.",
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
                                  // Check if the string contains a decimal point or scientific notation
                                  bool hasDecimalPoint = trimmed.find('.') != std::string::npos;
                                  bool hasExponent = trimmed.find('e') != std::string::npos || trimmed.find('E') != std::string::npos;
                                  bool hasFloatSuffix = (trimmed.back() == 'f' || trimmed.back() == 'F');
                                  
                                  // If it has float suffix, remove it for parsing
                                  std::string parseStr = trimmed;
                                  if (hasFloatSuffix) {
                                      parseStr = trimmed.substr(0, trimmed.length() - 1);
                                      hasDecimalPoint = parseStr.find('.') != std::string::npos;
                                      hasExponent = parseStr.find('e') != std::string::npos || parseStr.find('E') != std::string::npos;
                                  }
                                  
                                  // If no decimal point, no exponent, and no float suffix, try to parse as integer first
                                  if (!hasDecimalPoint && !hasExponent && !hasFloatSuffix) {
                                      try {
                                          size_t pos;
                                          long long intResult = std::stoll(parseStr, &pos);
                                          
                                          // Check if the entire string was consumed
                                          if (pos == parseStr.length()) {
                                              // Check if the value fits in int range
                                              if (intResult >= std::numeric_limits<int>::min() &&
                                                  intResult <= std::numeric_limits<int>::max()) {
                                                  return static_cast<int>(intResult);
                                              }
                                              // If too large for int, fall through to double
                                          }
                                      } catch (const std::out_of_range &) {
                                          // Number too large for long long, fall through to double
                                      } catch (const std::invalid_argument &) {
                                          // Not a valid integer, fall through to floating point parsing
                                      }
                                  }
                                  
                                  // Parse as floating point
                                  size_t pos;
                                  double doubleResult = std::stod(parseStr, &pos);
                                  
                                  // Check if the entire string was consumed
                                  if (pos != parseStr.length()) {
                                      throw Exception(name() + "::string_to_number invalid number format: '" + str + "'");
                                  }
                                  
                                  // Check for infinity and NaN
                                  if (std::isinf(doubleResult)) {
                                      throw Exception(name() + "::string_to_number result is infinity: '" + str + "'");
                                  }
                                  if (std::isnan(doubleResult)) {
                                      throw Exception(name() + "::string_to_number result is not a number: '" + str + "'");
                                  }
                                  
                                  // Determine if we should return float or double
                                  if (hasFloatSuffix) {
                                      // Explicit float suffix
                                      if (doubleResult >= -std::numeric_limits<float>::max() &&
                                          doubleResult <= std::numeric_limits<float>::max()) {
                                          return static_cast<float>(doubleResult);
                                      } else {
                                          throw Exception(name() + "::string_to_number float value out of range: '" + str + "'");
                                      }
                                  } else if (hasDecimalPoint || hasExponent) {
                                      // For floating point values, prefer double for better precision
                                      // unless it can be exactly represented as float
                                      float floatResult = static_cast<float>(doubleResult);
                                      if (static_cast<double>(floatResult) == doubleResult &&
                                          doubleResult >= -std::numeric_limits<float>::max() &&
                                          doubleResult <= std::numeric_limits<float>::max() &&
                                          (doubleResult == 0.0 || std::abs(doubleResult) >= 1e-7)) { // Avoid very small precision issues
                                          return floatResult;
                                      } else {
                                          return doubleResult;
                                      }
                                  }
                                  
                                  // Fallback to double
                                  return doubleResult;
                                  
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