#ifndef SYMBOL_VALUE_HPP
#define SYMBOL_VALUE_HPP

#include <algorithm>
#include <stdexcept>
#include <string>
#include <variant>

#include "VariableTypes.hpp"

namespace Symbols {

class Value {
  public:
    using Variant = std::variant<int, double, float, std::string, bool>;

    Value() = default;

    Value(int v) : value_(v) { type_ = Symbols::Variables::Type::INTEGER; }

    Value(double v) : value_(v) { type_ = Symbols::Variables::Type::DOUBLE; }

    Value(float v) : value_(v) { type_ = Symbols::Variables::Type::FLOAT; }

    Value(const std::string & v) : value_(v) { type_ = Symbols::Variables::Type::STRING; }

    Value(const char * v) : value_(std::string(v)) { type_ = Symbols::Variables::Type::STRING; }

    Value(bool v) : value_(v) { type_ = Symbols::Variables::Type::BOOLEAN; }

    Value(const std::string & str, bool autoDetectType) { *this = fromString(str, autoDetectType); }

    const Variant & get() const { return value_; }

    Symbols::Variables::Type getType() const { return type_; }

    Variant & get() { return value_; }

    template <typename T> T get() const { return std::get<T>(value_); }

    static Symbols::Value makeNull() {
        auto v = Value("null");
        return v;
    }

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

    static Value fromString(const std::string & str, bool autoDetectType) {
        if (!autoDetectType) {
            return fromStringToString(str);
        }

        std::string trimmed = str;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
        trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

        // Check bool literals
        std::string lower = trimmed;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "true" || lower == "false") {
            try {
                return fromStringToBool(trimmed);
            } catch (...) {
            }
        }

        // Check int
        try {
            size_t idx;
            int    i = std::stoi(trimmed, &idx);
            if (idx == trimmed.size()) {
                return fromStringToInt(trimmed);
            }
        } catch (...) {
        }

        // Check double
        try {
            size_t idx;
            double d = std::stod(trimmed, &idx);
            if (idx == trimmed.size()) {
                return fromStringToDouble(trimmed);
            }
        } catch (...) {
        }

        // Fallback
        return fromStringToString(str);
    }

    static Value fromString(const std::string & str, Symbols::Variables::Type type) {
        switch (type) {
            case Symbols::Variables::Type::INTEGER:
                return fromStringToInt(str);
            case Symbols::Variables::Type::DOUBLE:
                return fromStringToDouble(str);
            case Symbols::Variables::Type::FLOAT:
                return fromStringToFloat(str);
            case Symbols::Variables::Type::BOOLEAN:
                return fromStringToBool(str);
            case Symbols::Variables::Type::STRING:
            default:
                return fromStringToString(str);
        }
    }
  private:
    Variant                  value_;
    Symbols::Variables::Type type_;

    static Value fromStringToInt(const std::string & str) { return Value(std::stoi(str)); }

    static Value fromStringToDouble(const std::string & str) { return Value(std::stod(str)); }

    static Value fromStringToFloat(const std::string & str) { return Value(std::stof(str)); }

    static Value fromStringToBool(const std::string & str) {
        std::string s = str;
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        if (s == "true" || s == "1") {
            return Value(true);
        }
        if (s == "false" || s == "0") {
            return Value(false);
        }
        throw std::invalid_argument("Invalid bool string: " + str);
    }

    static Value fromStringToString(const std::string & str) { return Value(str); }
};

}  // namespace Symbols

#endif  // SYMBOL_VALUE_HPP
