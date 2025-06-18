#ifndef MODULES_MATHMODULE_HPP
#define MODULES_MATHMODULE_HPP

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include "../BaseModule.hpp"
#include "../../Symbols/SymbolContainer.hpp"
#include "../../Symbols/Value.hpp"
#include "../../Symbols/RegistrationMacros.hpp"

namespace Modules {

/**
 * @brief Math module providing mathematical functions and constants for VoidScript
 * 
 * Provides mathematical functions:
 * - PI() -> returns mathematical constant π
 * - ceil(number) -> ceiling function  
 * - floor(number) -> floor function
 * - round(number) -> round to nearest integer
 * - abs(number) -> absolute value
 * - sqrt(number) -> square root
 * - pow(base, exponent) -> power function
 * - sin(radians) -> sine function
 * - cos(radians) -> cosine function
 * - tan(radians) -> tangent function
 * - log(number) -> natural logarithm
 * - log10(number) -> base-10 logarithm
 * - min(a, b) -> minimum of two numbers
 * - max(a, b) -> maximum of two numbers
 */
class MathModule : public BaseModule {
  public:
    MathModule() {
        setModuleName("Math");
        setDescription("Provides comprehensive mathematical functions including trigonometric, logarithmic, and arithmetic operations, along with mathematical constants");
    }

    void registerFunctions() override {
        // PI constant function
        std::vector<Symbols::FunctionParameterInfo> no_params = {};
        REGISTER_FUNCTION("PI", Symbols::Variables::Type::DOUBLE, no_params,
                          "Mathematical constant π (pi)",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (!args.empty()) {
                                  throw Exception(name() + "::PI expects no arguments");
                              }
                              return M_PI;
                          });

        // ceil function
        std::vector<Symbols::FunctionParameterInfo> number_param = {
            { "number", Symbols::Variables::Type::DOUBLE, "The number to calculate ceiling of", false, false }
        };
        REGISTER_FUNCTION("ceil", Symbols::Variables::Type::INTEGER, number_param,
                          "Returns the smallest integer greater than or equal to the given number",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::ceil expects one argument");
                              }
                              double value = convertToDouble(args[0], "ceil");
                              return static_cast<int>(std::ceil(value));
                          });

        // floor function
        REGISTER_FUNCTION("floor", Symbols::Variables::Type::INTEGER, number_param,
                          "Returns the largest integer less than or equal to the given number",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::floor expects one argument");
                              }
                              double value = convertToDouble(args[0], "floor");
                              return static_cast<int>(std::floor(value));
                          });

        // round function
        REGISTER_FUNCTION("round", Symbols::Variables::Type::INTEGER, number_param,
                          "Returns the nearest integer to the given number",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::round expects one argument");
                              }
                              double value = convertToDouble(args[0], "round");
                              return static_cast<int>(std::round(value));
                          });

        // abs function
        REGISTER_FUNCTION("abs", Symbols::Variables::Type::DOUBLE, number_param,
                          "Returns the absolute value of the given number",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::abs expects one argument");
                              }
                              double value = convertToDouble(args[0], "abs");
                              return std::abs(value);
                          });

        // sqrt function
        REGISTER_FUNCTION("sqrt", Symbols::Variables::Type::DOUBLE, number_param,
                          "Returns the square root of the given number",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::sqrt expects one argument");
                              }
                              double value = convertToDouble(args[0], "sqrt");
                              if (value < 0) {
                                  throw Exception(name() + "::sqrt: cannot calculate square root of negative number");
                              }
                              return std::sqrt(value);
                          });

        // pow function
        std::vector<Symbols::FunctionParameterInfo> pow_params = {
            { "base", Symbols::Variables::Type::DOUBLE, "The base number", false, false },
            { "exponent", Symbols::Variables::Type::DOUBLE, "The exponent", false, false }
        };
        REGISTER_FUNCTION("pow", Symbols::Variables::Type::DOUBLE, pow_params,
                          "Returns base raised to the power of exponent",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 2) {
                                  throw Exception(name() + "::pow expects two arguments");
                              }
                              double base = convertToDouble(args[0], "pow");
                              double exponent = convertToDouble(args[1], "pow");
                              return std::pow(base, exponent);
                          });

        // sin function
        REGISTER_FUNCTION("sin", Symbols::Variables::Type::DOUBLE, number_param,
                          "Returns the sine of the given angle in radians",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::sin expects one argument");
                              }
                              double radians = convertToDouble(args[0], "sin");
                              return std::sin(radians);
                          });

        // cos function
        REGISTER_FUNCTION("cos", Symbols::Variables::Type::DOUBLE, number_param,
                          "Returns the cosine of the given angle in radians",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::cos expects one argument");
                              }
                              double radians = convertToDouble(args[0], "cos");
                              return std::cos(radians);
                          });

        // tan function
        REGISTER_FUNCTION("tan", Symbols::Variables::Type::DOUBLE, number_param,
                          "Returns the tangent of the given angle in radians",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::tan expects one argument");
                              }
                              double radians = convertToDouble(args[0], "tan");
                              return std::tan(radians);
                          });

        // log function (natural logarithm)
        REGISTER_FUNCTION("log", Symbols::Variables::Type::DOUBLE, number_param,
                          "Returns the natural logarithm of the given number",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::log expects one argument");
                              }
                              double value = convertToDouble(args[0], "log");
                              if (value <= 0) {
                                  throw Exception(name() + "::log: cannot calculate logarithm of non-positive number");
                              }
                              return std::log(value);
                          });

        // log10 function (base-10 logarithm)
        REGISTER_FUNCTION("log10", Symbols::Variables::Type::DOUBLE, number_param,
                          "Returns the base-10 logarithm of the given number",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::log10 expects one argument");
                              }
                              double value = convertToDouble(args[0], "log10");
                              if (value <= 0) {
                                  throw Exception(name() + "::log10: cannot calculate logarithm of non-positive number");
                              }
                              return std::log10(value);
                          });

        // min function
        std::vector<Symbols::FunctionParameterInfo> two_number_params = {
            { "a", Symbols::Variables::Type::DOUBLE, "First number", false, false },
            { "b", Symbols::Variables::Type::DOUBLE, "Second number", false, false }
        };
        REGISTER_FUNCTION("min", Symbols::Variables::Type::DOUBLE, two_number_params,
                          "Returns the minimum of two numbers",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 2) {
                                  throw Exception(name() + "::min expects two arguments");
                              }
                              double a = convertToDouble(args[0], "min");
                              double b = convertToDouble(args[1], "min");
                              return std::min(a, b);
                          });

        // max function
        REGISTER_FUNCTION("max", Symbols::Variables::Type::DOUBLE, two_number_params,
                          "Returns the maximum of two numbers",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 2) {
                                  throw Exception(name() + "::max expects two arguments");
                              }
                              double a = convertToDouble(args[0], "max");
                              double b = convertToDouble(args[1], "max");
                              return std::max(a, b);
                          });
    }

  private:
    /**
     * @brief Convert a ValuePtr to double, handling different numeric types
     * @param value The value to convert
     * @param functionName Name of the calling function for error messages
     * @return The value as a double
     */
    double convertToDouble(const Symbols::ValuePtr& value, const std::string& functionName) {
        switch (value.getType()) {
            case Symbols::Variables::Type::INTEGER:
                return static_cast<double>(value.get<int>());
            case Symbols::Variables::Type::FLOAT:
                return static_cast<double>(value.get<float>());
            case Symbols::Variables::Type::DOUBLE:
                return value.get<double>();
            default:
                throw Exception(name() + "::" + functionName + " expects a numeric argument");
        }
    }
};

} // namespace Modules

#endif // MODULES_MATHMODULE_HPP