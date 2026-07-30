#ifndef MODULES_MATHMODULE_HPP
#define MODULES_MATHMODULE_HPP

#include <cmath>
#include <random>
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
 * - exp(number) -> e raised to the given power
 * - E() -> Euler's number e
 * - rand_int(min, max) -> inclusive uniform random integer
 * - rand_double() -> uniform random double in [0, 1)
 * - rand_normal(mean, stddev) -> Gaussian (normal) random double
 * - rand_seed(seed) -> seed the random generator for reproducibility
 */
class MathModule : public BaseModule {
  public:
    MathModule() {
        setModuleName("Math");
        setDescription("Provides comprehensive mathematical functions including trigonometric, logarithmic, and arithmetic operations, along with mathematical constants");
        setBuiltIn(true);
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

        // exp function (e^x)
        REGISTER_FUNCTION("exp", Symbols::Variables::Type::DOUBLE, number_param,
                          "Returns e raised to the power of the given number",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::exp expects one argument");
                              }
                              double value = convertToDouble(args[0], "exp");
                              return std::exp(value);
                          });

        // E constant function (mirrors PI)
        REGISTER_FUNCTION("E", Symbols::Variables::Type::DOUBLE, no_params,
                          "Euler's number e (2.71828...)",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (!args.empty()) {
                                  throw Exception(name() + "::E expects no arguments");
                              }
                              return M_E;
                          });

        // rand_int(min, max) - inclusive uniform random integer
        std::vector<Symbols::FunctionParameterInfo> rand_int_params = {
            { "min", Symbols::Variables::Type::INTEGER, "Inclusive lower bound", false, false },
            { "max", Symbols::Variables::Type::INTEGER, "Inclusive upper bound", false, false }
        };
        REGISTER_FUNCTION("rand_int", Symbols::Variables::Type::INTEGER, rand_int_params,
                          "Returns a uniformly random integer in [min, max] (both inclusive)",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 2) {
                                  throw Exception(name() + "::rand_int expects two arguments");
                              }
                              long long lo = static_cast<long long>(convertToDouble(args[0], "rand_int"));
                              long long hi = static_cast<long long>(convertToDouble(args[1], "rand_int"));
                              if (lo > hi) {
                                  throw Exception(name() + "::rand_int: min must be <= max");
                              }
                              std::uniform_int_distribution<long long> dist(lo, hi);
                              return static_cast<int>(dist(rng_));
                          });

        // rand_double() - uniform random double in [0, 1)
        REGISTER_FUNCTION("rand_double", Symbols::Variables::Type::DOUBLE, no_params,
                          "Returns a uniformly random double in [0, 1)",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (!args.empty()) {
                                  throw Exception(name() + "::rand_double expects no arguments");
                              }
                              std::uniform_real_distribution<double> dist(0.0, 1.0);
                              return dist(rng_);
                          });

        // rand_normal(mean, stddev) - Gaussian random double
        std::vector<Symbols::FunctionParameterInfo> rand_normal_params = {
            { "mean",   Symbols::Variables::Type::DOUBLE, "Distribution mean", false, false },
            { "stddev", Symbols::Variables::Type::DOUBLE, "Standard deviation", false, false }
        };
        REGISTER_FUNCTION("rand_normal", Symbols::Variables::Type::DOUBLE, rand_normal_params,
                          "Returns a Gaussian (normal) random double with the given mean and stddev",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 2) {
                                  throw Exception(name() + "::rand_normal expects two arguments");
                              }
                              double mean   = convertToDouble(args[0], "rand_normal");
                              double stddev = convertToDouble(args[1], "rand_normal");
                              if (stddev < 0) {
                                  throw Exception(name() + "::rand_normal: stddev must be >= 0");
                              }
                              std::normal_distribution<double> dist(mean, stddev);
                              return dist(rng_);
                          });

        // rand_seed(seed) - seed the generator for reproducible sequences
        std::vector<Symbols::FunctionParameterInfo> rand_seed_params = {
            { "seed", Symbols::Variables::Type::INTEGER, "Seed value", false, false }
        };
        REGISTER_FUNCTION("rand_seed", Symbols::Variables::Type::NULL_TYPE, rand_seed_params,
                          "Seeds the random generator so subsequent rand_* calls are reproducible",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::rand_seed expects one argument");
                              }
                              long long seed = static_cast<long long>(convertToDouble(args[0], "rand_seed"));
                              rng_.seed(static_cast<std::mt19937_64::result_type>(seed));
                              return Symbols::ValuePtr::null();
                          });

        // atan2(y, x)
        REGISTER_FUNCTION("atan2", Symbols::Variables::Type::DOUBLE, two_number_params,
                          "Arc tangent of y/x using the signs of both to pick the quadrant",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 2) {
                                  throw Exception(name() + "::atan2 expects two arguments");
                              }
                              return std::atan2(convertToDouble(args[0], "atan2"), convertToDouble(args[1], "atan2"));
                          });

        // hypot(x, y)
        REGISTER_FUNCTION("hypot", Symbols::Variables::Type::DOUBLE, two_number_params,
                          "Euclidean distance sqrt(x*x + y*y) without overflow",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 2) {
                                  throw Exception(name() + "::hypot expects two arguments");
                              }
                              return std::hypot(convertToDouble(args[0], "hypot"), convertToDouble(args[1], "hypot"));
                          });

        // sign(x) -> -1, 0 or 1
        REGISTER_FUNCTION("sign", Symbols::Variables::Type::INTEGER, number_param,
                          "Sign of a number: -1, 0 or 1",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::sign expects one argument");
                              }
                              double v = convertToDouble(args[0], "sign");
                              return (v > 0) - (v < 0);
                          });

        // clamp(x, lo, hi)
        std::vector<Symbols::FunctionParameterInfo> clamp_params = {
            { "value", Symbols::Variables::Type::DOUBLE, "The value",       false, false },
            { "lo",    Symbols::Variables::Type::DOUBLE, "Lower bound",     false, false },
            { "hi",    Symbols::Variables::Type::DOUBLE, "Upper bound",     false, false }
        };
        REGISTER_FUNCTION("clamp", Symbols::Variables::Type::DOUBLE, clamp_params,
                          "Constrain a value to the inclusive range [lo, hi]",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 3) {
                                  throw Exception(name() + "::clamp expects three arguments");
                              }
                              double v  = convertToDouble(args[0], "clamp");
                              double lo = convertToDouble(args[1], "clamp");
                              double hi = convertToDouble(args[2], "clamp");
                              if (lo > hi) {
                                  throw Exception(name() + "::clamp: lo must be <= hi");
                              }
                              return std::min(std::max(v, lo), hi);
                          });

        // gcd(a, b) - integers
        REGISTER_FUNCTION("gcd", Symbols::Variables::Type::INTEGER, two_number_params,
                          "Greatest common divisor of two integers",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 2) {
                                  throw Exception(name() + "::gcd expects two arguments");
                              }
                              long long a = std::llabs(static_cast<long long>(convertToDouble(args[0], "gcd")));
                              long long b = std::llabs(static_cast<long long>(convertToDouble(args[1], "gcd")));
                              while (b != 0) {
                                  long long t = b;
                                  b = a % b;
                                  a = t;
                              }
                              return static_cast<int>(a);
                          });

        // lcm(a, b) - integers
        REGISTER_FUNCTION("lcm", Symbols::Variables::Type::INTEGER, two_number_params,
                          "Least common multiple of two integers",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 2) {
                                  throw Exception(name() + "::lcm expects two arguments");
                              }
                              long long a = std::llabs(static_cast<long long>(convertToDouble(args[0], "lcm")));
                              long long b = std::llabs(static_cast<long long>(convertToDouble(args[1], "lcm")));
                              if (a == 0 || b == 0) {
                                  return 0;
                              }
                              long long g = a;
                              long long h = b;
                              while (h != 0) {
                                  long long t = h;
                                  h = g % h;
                                  g = t;
                              }
                              return static_cast<int>((a / g) * b);
                          });

        // deg2rad / rad2deg
        REGISTER_FUNCTION("deg2rad", Symbols::Variables::Type::DOUBLE, number_param,
                          "Convert degrees to radians",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::deg2rad expects one argument");
                              }
                              return convertToDouble(args[0], "deg2rad") * M_PI / 180.0;
                          });
        REGISTER_FUNCTION("rad2deg", Symbols::Variables::Type::DOUBLE, number_param,
                          "Convert radians to degrees",
                          [this](Symbols::FunctionArguments& args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw Exception(name() + "::rad2deg expects one argument");
                              }
                              return convertToDouble(args[0], "rad2deg") * 180.0 / M_PI;
                          });
    }

  private:
    // Shared random generator. Seeded non-deterministically at construction; rand_seed()
    // makes a run reproducible. Script execution is single-threaded, so no lock is needed.
    std::mt19937_64 rng_{ std::random_device{}() };

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