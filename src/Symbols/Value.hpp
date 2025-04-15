#ifndef SYMBOL_VALUE_HPP
#define SYMBOL_VALUE_HPP

#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>

namespace Symbols {

class Value {
  public:
    using Variant = std::variant<int, double, float, std::string, bool>;

    Value() = default;

    Value(int v) : value_(v) {}

    Value(double v) : value_(v) {}

    Value(float v) : value_(v) {}

    Value(const std::string & v) : value_(v) {}

    Value(const char * v) : value_(std::string(v)) {}

    Value(bool v) : value_(v) {}

    const Variant & get() const { return value_; }

    Variant & get() { return value_; }

    template <typename T> T get() const { return std::get<T>(value_); }

    // operator+
    friend Value operator+(const Value & lhs, const Value & rhs) {
        return std::visit(
            [](auto && a, auto && b) -> Value {
                using A = std::decay_t<decltype(a)>;
                using B = std::decay_t<decltype(b)>;
                if constexpr ((std::is_arithmetic_v<A> && std::is_arithmetic_v<B>) ) {
                    return Value{ a + b };
                } else {
                    throw std::runtime_error("Invalid types for operator+");
                }
            },
            lhs.value_, rhs.value_);
    }

    // operator==
    friend bool operator==(const Value & lhs, const Value & rhs) { return lhs.value_ == rhs.value_; }

    // to_string helper (needed for string + int, etc.)
    static std::string to_string(const Variant & v) {
        return std::visit(
            [](auto && val) -> std::string {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, bool>) {
                    return val ? "true" : "false";
                } else if constexpr (std::is_same_v<T, std::string>) {
                    return val;
                } else {
                    return std::to_string(val);
                }
            },
            v);
    }

    static std::string to_string(const Value & val) { return to_string(val.value_); }

  private:
    Variant value_;
};

}  // namespace Symbols

#endif  // SYMBOL_VALUE_HPP
