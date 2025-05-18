#ifndef SYMBOL_VALUE_HPP
#define SYMBOL_VALUE_HPP

#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>  // std::enable_if_t, std::is_same_v, std::decay_t, std::is_lvalue_reference_v
#include <variant>

#include "Symbols/VariableTypes.hpp"  // Győződj meg róla, hogy ez a fájl létezik és helyes.

namespace Symbols {

class Value;
class ValuePtr;

// Az ObjectMap definíciója most ValuePtr-t használ
using ObjectMap = std::map<std::string, ValuePtr>;

class ValuePtr {
  public:
    ValuePtr() = default;

    ValuePtr(std::nullptr_t) : ptr_(nullptr) {}

    explicit ValuePtr(const std::shared_ptr<Value> & ptr) : ptr_(ptr) {}

    explicit ValuePtr(std::shared_ptr<Value> && ptr) : ptr_(std::move(ptr)) {}

    // --- Speciális tagfüggvények (Rule of Five) ---
    ValuePtr(const ValuePtr &)                 = default;
    ValuePtr(ValuePtr &&) noexcept             = default;
    ValuePtr & operator=(const ValuePtr &)     = default;
    ValuePtr & operator=(ValuePtr &&) noexcept = default;
    ~ValuePtr()                                = default;
    // --- Vége: Speciális tagfüggvények ---

    static ValuePtr create();

    template <typename T> static ValuePtr create(const T & val);

    template <typename T,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, ValuePtr> && !std::is_lvalue_reference_v<T>>>
    static ValuePtr create(T && val);

    static ValuePtr createObjectMap() { return create(Symbols::ObjectMap{}); }

    static ValuePtr createObjectMap(const Symbols::ObjectMap & val);
    static ValuePtr createObjectMap(Symbols::ObjectMap && val);

    Value * operator->() {
        if (!ptr_) {
            throw std::runtime_error("Dereferencing null ValuePtr (operator->)");
        }
        return ptr_.get();
    }

    const Value * operator->() const {
        if (!ptr_) {
            throw std::runtime_error("Dereferencing null ValuePtr (const operator->)");
        }
        return ptr_.get();
    }

    Value & operator*() {
        if (!ptr_) {
            throw std::runtime_error("Dereferencing null ValuePtr (operator*)");
        }
        return *ptr_;
    }

    const Value & operator*() const {
        if (!ptr_) {
            throw std::runtime_error("Dereferencing null ValuePtr (const operator*)");
        }
        return *ptr_;
    }

    std::shared_ptr<Value> & raw() { return ptr_; }

    const std::shared_ptr<Value> & raw() const { return ptr_; }

    // Értékadások más típusokból
    ValuePtr & operator=(const std::shared_ptr<Value> & rhs) {
        ptr_ = rhs;
        return *this;
    }

    ValuePtr & operator=(std::shared_ptr<Value> && rhs) {
        ptr_ = std::move(rhs);
        return *this;
    }

    ValuePtr & operator=(const Symbols::ObjectMap & obj);

    operator Symbols::ObjectMap() const;

    explicit operator bool() const { return static_cast<bool>(ptr_); }

  private:
    std::shared_ptr<Value> ptr_;

    friend bool operator==(const ValuePtr & lhs, Variables::Type t);
    friend bool operator!=(const ValuePtr & lhs, Variables::Type t);
};

class Value {
  public:
    using Variant = std::variant<int, double, float, std::string, bool, Symbols::ObjectMap>;

    Value() : value_(nullptr), type_(Variables::Type::NULL_TYPE), is_null(true) {}

    Value(const unsigned char * uc, int len = -1);
    Value(int v);
    Value(size_t v);
    Value(double v);
    Value(float v);
    Value(const std::string & v);
    Value(const char * v);
    Value(bool v);
    Value(const Symbols::ObjectMap & v);
    Value(Symbols::ObjectMap && v);

    Variables::Type getType() const { return type_; }

    bool isNULL() const { return is_null || !value_; }

    template <typename T> T get() const {
        if (isNULL()) {
            throw std::runtime_error("Attempt to get value from a null Value object");
        }
        if (!value_) {
            throw std::runtime_error("Internal error: Value is marked not-null but has no variant data");
        }
        try {
            return std::get<T>(*value_);
        } catch (const std::bad_variant_access & e) {
            throw std::invalid_argument(std::string("Value not holding the requested type. Variant error: ") +
                                        e.what());
        }
    }

    const Variant & getVariant() const {
        if (!value_) {
            throw std::runtime_error("Accessing variant of a null Value");
        }
        return *value_;
    }

    Variant & getVariant() {
        if (!value_) {
            throw std::runtime_error("Accessing variant of a null Value");
        }
        return *value_;
    }

    void setNULL() {
        this->is_null = true;
        this->value_  = nullptr;
        this->type_   = Symbols::Variables::Type::NULL_TYPE;
    }

    static ValuePtr               null();
    static ValuePtr               makeNull(Variables::Type type);
    [[nodiscard]] static ValuePtr makeClassInstance(const Symbols::ObjectMap & v);
    [[nodiscard]] static ValuePtr makeClassInstance(Symbols::ObjectMap && v);

    operator int() const;
    operator double() const;
    operator float() const;
    operator std::string() const;
    operator bool() const;
    operator Symbols::ObjectMap() const;


    friend bool operator==(const Value & lhs, Variables::Type t) { return lhs.type_ == t; }

    friend bool operator!=(const Value & lhs, Variables::Type t) { return lhs.type_ != t; }

    friend bool  operator==(const Value & lhs, const Value & rhs);
    friend Value operator+(const Value & lhs, const Value & rhs);

    static std::string to_string(const Variant & v);         // Variant most Symbols::ObjectMap-et tartalmazhat
    static std::string to_string(const ValuePtr & val_ptr);  // Parameter ValuePtr

    static ValuePtr fromString(const std::string & str, bool autoDetectType = true);
    static ValuePtr fromString(const std::string & str, Variables::Type type);

  private:
    std::shared_ptr<Variant> value_;
    Variables::Type          type_;
    bool                     is_null = false;

    static ValuePtr fromStringToInt(const std::string & str);
    static ValuePtr fromStringToDouble(const std::string & str);
    static ValuePtr fromStringToFloat(const std::string & str);
    static ValuePtr fromStringToBool(const std::string & str);
    static ValuePtr fromStringToString(const std::string & str);
};

// --- ValuePtr create implementációk (Value teljes definíciója után) ---
inline ValuePtr ValuePtr::create() {
    return ValuePtr(std::make_shared<Value>());
}

template <typename T> inline ValuePtr ValuePtr::create(const T & val) {
    return ValuePtr(std::make_shared<Value>(val));
}

template <typename T, typename> inline ValuePtr ValuePtr::create(T && val) {
    return ValuePtr(std::make_shared<Value>(std::forward<T>(val)));
}

inline ValuePtr ValuePtr::createObjectMap(const Symbols::ObjectMap & val) {
    return ValuePtr(std::make_shared<Value>(val));
}

inline ValuePtr ValuePtr::createObjectMap(Symbols::ObjectMap && val) {
    return ValuePtr(std::make_shared<Value>(std::move(val)));
}

inline ValuePtr & ValuePtr::operator=(const Symbols::ObjectMap & obj) {
    ptr_ = std::make_shared<Value>(obj);
    return *this;
}

// --- Implementációk ... (a Value tagfüggvények implementációi innen folytatódnak, ahogy az előző válaszban) ---
// Value konstruktorok
inline Value::Value(const unsigned char * uc, int len) : is_null(false) {
    if (uc == nullptr) {
        setNULL();
        return;
    }
    if (len > -1) {
        value_ = std::make_shared<Variant>(std::string(reinterpret_cast<const char *>(uc), len));
    } else {
        value_ = std::make_shared<Variant>(std::string(reinterpret_cast<const char *>(uc)));
    }
    type_ = Symbols::Variables::Type::STRING;
}

inline Value::Value(int v) :
    value_(std::make_shared<Variant>(v)),
    type_(Symbols::Variables::Type::INTEGER),
    is_null(false) {}

inline Value::Value(size_t v) :
    value_(std::make_shared<Variant>(static_cast<int>(v))),
    type_(Symbols::Variables::Type::INTEGER),
    is_null(false) {}

inline Value::Value(double v) :
    value_(std::make_shared<Variant>(v)),
    type_(Symbols::Variables::Type::DOUBLE),
    is_null(false) {}

inline Value::Value(float v) :
    value_(std::make_shared<Variant>(v)),
    type_(Symbols::Variables::Type::FLOAT),
    is_null(false) {}

inline Value::Value(const std::string & v) :
    value_(std::make_shared<Variant>(v)),
    type_(Symbols::Variables::Type::STRING),
    is_null(false) {}

inline Value::Value(const char * v) :
    value_(std::make_shared<Variant>(std::string(v))),
    type_(Symbols::Variables::Type::STRING),
    is_null(false) {}

inline Value::Value(bool v) :
    value_(std::make_shared<Variant>(v)),
    type_(Symbols::Variables::Type::BOOLEAN),
    is_null(false) {}

inline Value::Value(const Symbols::ObjectMap & v) :
    value_(std::make_shared<Variant>(v)),
    type_(Symbols::Variables::Type::OBJECT),
    is_null(false) {}

inline Value::Value(Symbols::ObjectMap && v) :
    value_(std::make_shared<Variant>(std::move(v))),
    type_(Symbols::Variables::Type::OBJECT),
    is_null(false) {}

// Value statikus factory metódusok
inline ValuePtr Value::null() {
    return ValuePtr(std::make_shared<Value>());
}

inline ValuePtr Value::makeNull(Variables::Type type) {
    auto val_ptr = std::make_shared<Value>();
    val_ptr->setNULL();
    val_ptr->type_ = type;
    return ValuePtr(val_ptr);
}

inline ValuePtr Value::makeClassInstance(const Symbols::ObjectMap & v) {
    auto nv_ptr     = std::make_shared<Value>(v);
    nv_ptr->type_   = Symbols::Variables::Type::CLASS;
    nv_ptr->is_null = false;
    return ValuePtr(nv_ptr);
}

inline ValuePtr Value::makeClassInstance(Symbols::ObjectMap && v) {
    auto nv_ptr     = std::make_shared<Value>(std::move(v));
    nv_ptr->type_   = Symbols::Variables::Type::CLASS;
    nv_ptr->is_null = false;
    return ValuePtr(nv_ptr);
}

// Value konverziós operátorok
inline Value::operator int() const {
    if (isNULL()) {
        throw std::invalid_argument("Value is null, cannot convert to int");
    }
    if (std::holds_alternative<int>(*value_)) {
        return std::get<int>(*value_);
    }
    if (std::holds_alternative<double>(*value_)) {
        return static_cast<int>(std::get<double>(*value_));
    }
    if (std::holds_alternative<float>(*value_)) {
        return static_cast<int>(std::get<float>(*value_));
    }
    throw std::invalid_argument("Value not holding int or not convertible to int");
}

inline Value::operator double() const {
    if (isNULL()) {
        throw std::invalid_argument("Value is null, cannot convert to double");
    }
    if (std::holds_alternative<double>(*value_)) {
        return std::get<double>(*value_);
    }
    if (std::holds_alternative<int>(*value_)) {
        return static_cast<double>(std::get<int>(*value_));
    }
    if (std::holds_alternative<float>(*value_)) {
        return static_cast<double>(std::get<float>(*value_));
    }
    throw std::invalid_argument("Value not holding double or not convertible to double");
}

inline Value::operator float() const {
    if (isNULL()) {
        throw std::invalid_argument("Value is null, cannot convert to float");
    }
    if (std::holds_alternative<float>(*value_)) {
        return std::get<float>(*value_);
    }
    if (std::holds_alternative<int>(*value_)) {
        return static_cast<float>(std::get<int>(*value_));
    }
    if (std::holds_alternative<double>(*value_)) {
        return static_cast<float>(std::get<double>(*value_));
    }
    throw std::invalid_argument("Value not holding float or not convertible to float");
}

inline Value::operator std::string() const {
    if (isNULL()) {
        return "null";
    }
    return Value::to_string(*value_);
}

inline Value::operator bool() const {
    if (isNULL()) {
        return false;
    }
    if (std::holds_alternative<bool>(*value_)) {
        return std::get<bool>(*value_);
    }
    if (std::holds_alternative<int>(*value_)) {
        return std::get<int>(*value_) != 0;
    }
    if (std::holds_alternative<double>(*value_)) {
        return std::get<double>(*value_) != 0.0;
    }
    if (std::holds_alternative<float>(*value_)) {
        return std::get<float>(*value_) != 0.0f;
    }
    if (std::holds_alternative<std::string>(*value_)) {
        return !std::get<std::string>(*value_).empty();
    }
    if (std::holds_alternative<Symbols::ObjectMap>(*value_)) {
        return true;
    }
    return false;
}

inline Value::operator Symbols::ObjectMap() const {
    if (isNULL() || !value_ || !std::holds_alternative<Symbols::ObjectMap>(*value_)) {
        throw std::invalid_argument("Value not holding ObjectMap or is null");
    }
    return std::get<Symbols::ObjectMap>(*value_);
}

// Value műveletek
inline bool operator==(const Value & lhs, const Value & rhs) {
    if (lhs.isNULL() && rhs.isNULL()) {
        return true;
    }
    if (lhs.isNULL() || rhs.isNULL()) {
        return false;
    }
    if (lhs.type_ != rhs.type_) {
        return false;
    }
    if (!lhs.value_ || !rhs.value_) {
        return false;
    }
    return *lhs.value_ == *rhs.value_;
}

inline Value operator+(const Value & lhs, const Value & rhs) {
    if (lhs.isNULL() || rhs.isNULL() || !lhs.value_ || !rhs.value_) {
        throw std::runtime_error("Arithmetic operation on null Value");
    }
    return std::visit(
        [](auto && a, auto && b) -> Value {
            using A = std::decay_t<decltype(a)>;
            using B = std::decay_t<decltype(b)>;
            if constexpr (std::is_arithmetic_v<A> && std::is_arithmetic_v<B>) {
                if constexpr (std::is_same_v<A, double> || std::is_same_v<B, double>) {
                    return Value{ static_cast<double>(a) + static_cast<double>(b) };
                } else if constexpr (std::is_same_v<A, float> || std::is_same_v<B, float>) {
                    return Value{ static_cast<float>(a) + static_cast<float>(b) };
                } else {
                    return Value{ static_cast<int>(a) + static_cast<int>(b) };
                }
            } else if constexpr (std::is_same_v<A, std::string> && std::is_same_v<B, std::string>) {
                return Value{ a + b };
            } else if constexpr (std::is_same_v<A, Symbols::ObjectMap> || std::is_same_v<B, Symbols::ObjectMap>) {
                throw std::runtime_error("Cannot use operator+ on ObjectMap type");
            } else {
                throw std::runtime_error("Invalid types for operator+");
            }
        },
        *lhs.value_, *rhs.value_);
}

// Value::to_string implementációk
inline std::string Value::to_string(const Variant & v) {
    return std::visit(
        [](auto && val) -> std::string {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, bool>) {
                return val ? "true" : "false";
            } else if constexpr (std::is_same_v<T, std::string>) {
                return val;
            } else if constexpr (std::is_same_v<T, Symbols::ObjectMap>) {
                return "[object]";
            } else {
                return std::to_string(val);
            }
        },
        v);
}

inline std::string Value::to_string(const ValuePtr & val_ptr) {
    if (!val_ptr.raw() || val_ptr->isNULL()) {
        return "null";
    }
    if (!val_ptr->value_) {
        return "null";
    }
    return to_string(*(val_ptr->value_));
}

// Value::fromString implementációk
inline ValuePtr Value::fromStringToInt(const std::string & str) {
    return ValuePtr(std::make_shared<Value>(std::stoi(str)));
}

inline ValuePtr Value::fromStringToString(const std::string & str) {
    return ValuePtr(std::make_shared<Value>(str));
}

inline ValuePtr Value::fromStringToDouble(const std::string & str) {
    return ValuePtr(std::make_shared<Value>(std::stod(str)));
}

inline ValuePtr Value::fromStringToFloat(const std::string & str) {
    return ValuePtr(std::make_shared<Value>(std::stof(str)));
}

inline ValuePtr Value::fromStringToBool(const std::string & str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "true" || s == "1") {
        return ValuePtr(std::make_shared<Value>(true));
    }
    if (s == "false" || s == "0") {
        return ValuePtr(std::make_shared<Value>(false));
    }
    throw std::invalid_argument("Invalid bool string: " + str);
}

inline ValuePtr Value::fromString(const std::string & str, bool autoDetectType) {
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

inline ValuePtr Value::fromString(const std::string & str, Variables::Type type) {
    switch (type) {
        case Variables::Type::INTEGER:
            return fromStringToInt(str);
        case Variables::Type::DOUBLE:
            return fromStringToDouble(str);
        case Variables::Type::FLOAT:
            return fromStringToFloat(str);
        case Variables::Type::BOOLEAN:
            return fromStringToBool(str);
        case Variables::Type::STRING:
        default:
            return fromStringToString(str);
    }
}

// ValuePtr operátorok implementációja
inline ValuePtr::operator Symbols::ObjectMap() const {
    if (!ptr_) {
        throw std::runtime_error("Cannot convert null ValuePtr to ObjectMap");
    }
    return static_cast<Symbols::ObjectMap>(*ptr_);
}

inline bool operator==(const ValuePtr & lhs, Variables::Type t) {
    if (!lhs.raw()) {
        return (t == Variables::Type::NULL_TYPE || t == Variables::Type::UNDEFINED_TYPE);
    }
    return lhs->getType() == t;
}

inline bool operator!=(const ValuePtr & lhs, Variables::Type t) {
    return !(lhs == t);
}

}  // namespace Symbols

#endif  // SYMBOL_VALUE_HPP
