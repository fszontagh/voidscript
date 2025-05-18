#ifndef SYMBOL_VALUE_HPP
#define SYMBOL_VALUE_HPP

#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>

#include "Symbols/VariableTypes.hpp"

namespace Symbols {

class Value {
  public:
    using ObjectMap  = std::map<std::string, std::shared_ptr<Symbols::Value>>;
    using Variant    = std::variant<int, double, float, std::string, bool, ObjectMap>;
    using VariantPtr = std::shared_ptr<Variant>;
    using ValuePtr   = std::shared_ptr<Value>;

    static ValuePtr create() { return std::make_shared<Symbols::Value>(); }

    template <typename T> static ValuePtr create(const T & initval) {
        return std::make_shared<Symbols::Value>(initval);
    }

    /**
    * @brief Creates an empty object map
    */
    static Symbols::Value::ValuePtr CreateObjectMap() { return Value::create(Symbols::Value::ObjectMap{}); }

    static Symbols::Value::ValuePtr CreateObjectMap(const ObjectMap & val) { return Value::create(val); }

    /**
     * @brief Default-constructed value is undefined.
     */
    Value() : value_(nullptr), type_(Symbols::Variables::Type::UNDEFINED_TYPE) {}

    Value(const unsigned char * uc, int len = -1) {
        if (uc == nullptr) {
            value_  = nullptr;
            is_null = true;
            return;
        }

        if (len > -1) {
            value_ = std::make_shared<Variant>(std::string(reinterpret_cast<const char *>(uc), len));
            type_  = Symbols::Variables::Type::STRING;
            return;
        }

        value_ = std::make_shared<Variant>(std::string(reinterpret_cast<const char *>(uc)));
        type_  = Symbols::Variables::Type::STRING;
    }

    Value(int v) : value_(std::make_shared<Variant>(v)) { type_ = Symbols::Variables::Type::INTEGER; }

    Value(size_t v) : value_(std::make_shared<Variant>(static_cast<int>(v))) {
        type_ = Symbols::Variables::Type::INTEGER;
    }

    Value(double v) : value_(std::make_shared<Variant>(v)) { type_ = Symbols::Variables::Type::DOUBLE; }

    Value(float v) : value_(std::make_shared<Variant>(v)) { type_ = Symbols::Variables::Type::FLOAT; }

    Value(const std::string & v) : value_(std::make_shared<Variant>(v)) { type_ = Symbols::Variables::Type::STRING; }

    Value(const char * v) : value_(std::make_shared<Variant>(std::string(v))) {
        type_ = Symbols::Variables::Type::STRING;
    }

    Value(bool v) : value_(std::make_shared<Variant>(v)) { type_ = Symbols::Variables::Type::BOOLEAN; }

    Value(const ObjectMap & v) : value_(std::make_shared<Variant>(v)) { type_ = Symbols::Variables::Type::OBJECT; }

    //Value(const std::string & str, bool autoDetectType) { this = fromString(str, autoDetectType); }

    const Variant & get() const { return *value_; }

    Variant & get() { return *value_; }

    Symbols::Variables::Type getType() const { return type_; }

    template <typename T> T get() const { return std::get<T>(*value_); }

    static Symbols::Value::ValuePtr null() {
        auto v     = std::make_shared<Symbols::Value>();
        v->is_null = true;
        v->type_   = Symbols::Variables::Type::NULL_TYPE;
        return v;
    }

    static Symbols::Value::ValuePtr makeNull(const Variables::Type & type) {
        auto v     = std::make_shared<Symbols::Value>();
        v->is_null = true;
        v->type_   = type;
        return v;
    }

    void setNULL() { this->is_null = true; }

    [[nodiscard]] static Symbols::Value::ValuePtr makeClassInstance(const ObjectMap & v) {
        auto nv   = std::make_shared<Symbols::Value>(v);
        nv->type_ = Symbols::Variables::Type::CLASS;
        return nv;
    }

    operator int() const {
        if (std::holds_alternative<int>(*value_)) {
            return std::get<int>(*value_);
        }
        throw std::invalid_argument("Value not holding int");
    }

    operator double() const {
        if (std::holds_alternative<double>(*value_)) {
            return std::get<double>(*value_);
        }
        throw std::invalid_argument("Value not holding double");
    }

    operator float() const {
        if (std::holds_alternative<float>(*value_)) {
            return std::get<float>(*value_);
        }
        throw std::invalid_argument("Value not holding float");
    }

    operator std::string() const {
        if (std::holds_alternative<std::string>(*value_)) {
            return std::get<std::string>(*value_);
        }
        throw std::invalid_argument("Value not holding std::string");
    }

    operator bool() const {
        if (std::holds_alternative<bool>(*value_)) {
            return std::get<bool>(*value_);
        }
        throw std::invalid_argument("Value not holding bool");
    }

    //
    friend bool operator==(const Symbols::Value::ValuePtr & lhs, const Symbols::Variables::Type & t) {
        return lhs->type_ == t;
    }

    friend bool operator!=(const Symbols::Value::ValuePtr & lhs, const Symbols::Variables::Type & t) {
        return lhs->type_ != t;
    }


    operator Symbols::Value::ObjectMap() const {
        if (std::holds_alternative<Symbols::Value::ObjectMap>(*value_)) {
            return std::get<Symbols::Value::ObjectMap>(*value_);
        }
        throw std::invalid_argument("Value not holding Symbols::Value::ObjectMap");
    }

    Symbols::Value * operator->() { return this; }

    const Symbols::Value * operator->() const { return this; }

    friend Value operator+(const Value & lhs, const Value & rhs) {
        return std::visit(
            [](auto && a, auto && b) -> Value {
                using A = std::decay_t<decltype(a)>;
                using B = std::decay_t<decltype(b)>;
                if constexpr (std::is_arithmetic_v<A> && std::is_arithmetic_v<B>) {
                    return Value{ a + b };
                } else {
                    throw std::runtime_error("Invalid types for operator+");
                }
            },
            *lhs.value_, *rhs.value_);
    }

    friend bool operator==(const Value & lhs, const Value & rhs) {
        if (!lhs.value_ || !rhs.value_) {
            return lhs.value_ == rhs.value_;
        }
        return *lhs.value_ == *rhs.value_;
    }

    static std::string to_string(const Variant & v) {
        return std::visit(
            [](auto && val) -> std::string {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, bool>) {
                    return val ? "true" : "false";
                } else if constexpr (std::is_same_v<T, std::string>) {
                    return val;
                } else if constexpr (std::is_same_v<T, ObjectMap>) {
                    return "[object]";
                } else {
                    return std::to_string(val);
                }
            },
            v);
    }

    static std::string to_string(const Value::ValuePtr & val) {
        if (!val->value_) {
            return "null";
        }
        return to_string(*val->value_);
    }

    static Symbols::Value::ValuePtr fromString(const std::string & str, bool autoDetectType) {
        if (!autoDetectType) {
            return fromStringToString(str);
        }

        std::string trimmed = str;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
        trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

        std::string lower = trimmed;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "true" || lower == "false") {
            try {
                return fromStringToBool(trimmed);
            } catch (...) {
            }
        }

        try {
            size_t idx;
            int    i = std::stoi(trimmed, &idx);
            if (idx == trimmed.size()) {
                return fromStringToInt(trimmed);
            }
        } catch (...) {
        }

        try {
            size_t idx;
            double d = std::stod(trimmed, &idx);
            if (idx == trimmed.size()) {
                return fromStringToDouble(trimmed);
            }
        } catch (...) {
        }

        return fromStringToString(str);
    }

    static Symbols::Value::ValuePtr fromString(const std::string & str, Symbols::Variables::Type type) {
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

    bool isNULL() const { return is_null; }

  private:
    std::shared_ptr<Variant> value_;
    Symbols::Variables::Type type_;
    bool                     is_null = false;

    static Value::ValuePtr fromStringToInt(const std::string & str) {
        return std::make_shared<Symbols::Value>(std::stoi(str));
    }

    static Value::ValuePtr fromStringToDouble(const std::string & str) {
        return std::make_shared<Symbols::Value>(std::stod(str));
    }

    static Value::ValuePtr fromStringToFloat(const std::string & str) {
        return std::make_shared<Symbols::Value>(std::stof(str));
    }

    static Value::ValuePtr fromStringToBool(const std::string & str) {
        std::string s = str;
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        if (s == "true" || s == "1") {
            return std::make_shared<Symbols::Value>(true);
        }
        if (s == "false" || s == "0") {
            return std::make_shared<Symbols::Value>(false);
        }
        throw std::invalid_argument("Invalid bool string: " + str);
    }

    static Value::ValuePtr fromStringToString(const std::string & str) { return std::make_shared<Value>(str); }
};

}  // namespace Symbols

#endif  // SYMBOL_VALUE_HPP
