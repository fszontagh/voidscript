#ifndef SYMBOLS_VALUE_HPP
#define SYMBOLS_VALUE_HPP

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>

#include "Symbols/VariableTypes.hpp"

namespace Symbols {

class Value;
class ValuePtr;

using ObjectMap = std::map<std::string, ValuePtr>;

class Value {
    friend class ValuePtr;

    template <typename T> const T & get() const {
        if (type_id_ != typeid(T)) {
            throw std::bad_cast();
        }
        return *std::static_pointer_cast<T>(data_);
    }

    void setNULL() { is_null = true; }
  private:
    Symbols::Variables::Type type_ = Variables::Type::NULL_TYPE;
    std::shared_ptr<void>    data_;
    std::type_index          type_id_ = typeid(void);
    bool                     is_null  = false;

    template <typename T> void set(T value) {
        data_    = std::make_shared<T>(std::move(value));
        type_id_ = typeid(T);
        is_null  = false;
    }

  public:
    Value() = default;

    bool isNULL() const { return is_null; }

    template <typename T> T & get() {
        if (type_id_ != typeid(T)) {
            throw std::bad_cast();
        }
        return *std::static_pointer_cast<T>(data_);
    }

    Symbols::Variables::Type getType() const { return type_; }

    std::string toString() const {
        if (type_ == Variables::Type::STRING) {
            return get<std::string>();
        }
        switch (type_) {
            case Variables::Type::INTEGER:  return std::to_string(get<int>());
            case Variables::Type::FLOAT:    return std::to_string(get<float>());
            case Variables::Type::DOUBLE:   return std::to_string(get<double>());
            case Variables::Type::BOOLEAN:  return get<bool>() ? "true" : "false";
            default:                         return "null";
        }
    }
};

class ValuePtr {
  private:
    std::shared_ptr<Value> ptr_;

     ValuePtr(const ValuePtr & other) {
        ptr_ = other.registryLookup(other.ptr_);
    }

    ValuePtr & operator=(const ValuePtr & other) {
        ptr_ = other.registryLookup(other.ptr_);
        return *this;
    }

    static std::mutex registryMutex_;
    static std::map<std::string, ValuePtr> valueRegistry_;

    static std::string valueToString(const Value& value) {
        if (value.type_ == Variables::Type::STRING) {
            return value.get<std::string>();
        }
        switch (value.type_) {
            case Variables::Type::INTEGER:  return std::to_string(value.get<int>());
            case Variables::Type::FLOAT:    return std::to_string(value.get<float>());
            case Variables::Type::DOUBLE:   return std::to_string(value.get<double>());
            case Variables::Type::BOOLEAN:  return value.get<bool>() ? "true" : "false";
            default:                         return "null";
        }
    }

    static std::string generateRegistryKey(const Value& value) {
        if (value.type_ == Variables::Type::OBJECT) {
            std::string key = "OBJECT:";
            for (const auto& [k, v] : value.get<ObjectMap>()) {
                key += k + ":" + ValuePtr::valueToString(*v);
            }
            return key;
        }
        return valueToString(value);
    }

    static ValuePtr& getFromRegistry(const Value& value) {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto key = generateRegistryKey(value);
        if (valueRegistry_.contains(key)) {
            return valueRegistry_.at(key);
        }
        // Create new ValuePtr from existing Value
        auto newPtr = std::make_shared<ValuePtr>(value);
        valueRegistry_[key] = newPtr;
        return *newPtr;
    }

    ValuePtr& registryLookup(Value& value) {
        static ValuePtr nullPtr;
        if (valueRegistry_.empty()) {
            return nullPtr;
        }
        auto it = valueRegistry_.find(generateRegistryKey(value));
        if (it != valueRegistry_.end()) {
            ptr_ = it->second;
        }
        return *this;
    }

    void ensure_object() {
        if (!ptr_) {
            ptr_ = std::make_shared<Value>();
        }
        if (ptr_->type_ != Variables::Type::OBJECT) {
            ptr_->set<ObjectMap>({});
            ptr_->type_ = Variables::Type::OBJECT;
        }
    }

  public:
    ValuePtr() : ptr_(std::make_shared<Value>()) { ptr_->setNULL(); }

    ValuePtr(const ValuePtr &)                 = default;
    ValuePtr & operator=(const ValuePtr & other) noexcept {
        if (this != &other) {
            ptr_ = other.ptr_;
        }
        return *this;
    }

    ValuePtr & operator=(ValuePtr && other) noexcept {
        ptr_ = std::move(other.ptr_);
        return *this;
    }
    ValuePtr(ValuePtr &&) noexcept             = default;
    ValuePtr & operator=(ValuePtr &&) noexcept = default;

    ValuePtr(const Value& value) {
        ptr_ = createRegistryEntry(value);
    }

    ValuePtr(int v) {
        ptr_ = std::make_shared<Value>();
        ptr_->set(v);
        ptr_->type_ = Variables::Type::INTEGER;
    }

    ValuePtr(float v) {
        ptr_ = std::make_shared<Value>();
        ptr_->set(v);
        ptr_->type_ = Variables::Type::FLOAT;
    }

    ValuePtr(double v) {
        ptr_ = std::make_shared<Value>();
        ptr_->set(v);
        ptr_->type_ = Variables::Type::DOUBLE;
    }

    ValuePtr(bool v) {
        ptr_ = std::make_shared<Value>();
        ptr_->set(v);
        ptr_->type_ = Variables::Type::BOOLEAN;
    }

    ValuePtr(const std::string & v) {
        ptr_ = std::make_shared<Value>();
        ptr_->set(v);
        ptr_->type_ = Variables::Type::STRING;
    }

    /*
    ValuePtr(const char * v) {
        if ()
        ptr_ = std::make_shared<Value>();
        ptr_->set(std::string(v));
        ptr_->type_ = Variables::Type::STRING;
    }
*/
    ValuePtr(const ObjectMap & map) {
        Value val;
        val.set(map);
        val.type_ = Variables::Type::OBJECT;
        ptr_ = createRegistryEntry(val);
    }

    template <typename T>
    static ValuePtr create(T&& value) {
        Value val = std::forward<T>(value);
        return ValuePtr(val);
    }

    ValuePtr(Value&& value) {
        ptr_ = createRegistryEntry(value);
    }

    Variables::Type getType() const { return ptr_ ? ptr_->type_ : Variables::Type::NULL_TYPE; }

    void setType(Symbols::Variables::Type type) {
        ensure_object();
        ptr_->type_ = type;
    }

    ValuePtr & setNULL() {
        ensure_object();
        ptr_->setNULL();
        return *this;
    }

    static ValuePtr null(Symbols::Variables::Type type) {
        auto z = ValuePtr(0);
        z->setNULL();
        z.setType(type);
        return z;
    }
    static ValuePtr null() {
        auto z = ValuePtr(0);
        z->setNULL();
        return z;
    }

    std::shared_ptr<Value> operator->() const { return ptr_; }

    template <typename T> T & get() { return ptr_->get<T>(); }

    template <typename T> const T & get() const { return ptr_->get<T>(); }

    operator Symbols::Variables::Type() { return ptr_->getType(); }

    operator Symbols::Variables::Type() const { return ptr_->getType(); }

    bool operator==(Symbols::Variables::Type type) const { return ptr_->type_ == type; }

    bool operator!=(Symbols::Variables::Type type) const { return ptr_->type_ != type; }

    template <typename T>
    requires(std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_same_v<T, double> ||
             std::is_same_v<T, ObjectMap> || std::is_same_v<T, bool> ||
             std::is_same_v<T, std::string>) operator T() const {
        return ptr_->get<T>();
    }

    static ValuePtr makeClassInstance(const ObjectMap & v) {
        auto _class   = Symbols::ValuePtr(v);
        _class->type_ = Symbols::Variables::Type::CLASS;  //setType(Symbols::Variables::Type::CLASS);
        return _class;
    }

    // objektumszerű indexelés
    ValuePtr & operator[](const std::string & key) {
        ensure_object();
        ObjectMap & map = ptr_->get<ObjectMap>();
        return map[key];
    }

    // converts all values into string representation. Don't use as value like expecting string. This is not a value getter
    std::string toString() const {
        if (!ptr_) {
            return "null";
        }
        switch (ptr_->type_) {
            case Variables::Type::INTEGER:
                return std::to_string(ptr_->get<int>());
            case Variables::Type::FLOAT:
                return std::to_string(ptr_->get<float>());
            case Variables::Type::DOUBLE:
                return std::to_string(ptr_->get<double>());
            case Variables::Type::BOOLEAN:
                return ptr_->get<bool>() ? "true" : "false";
            case Variables::Type::STRING:
                return ptr_->get<std::string>();
            case Variables::Type::OBJECT:
                return "{...}";
            case Variables::Type::NULL_TYPE:
            default:
                return "null";
        }
    }

    static ValuePtr fromString(const std::string & str) {
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
        Value val;
        val.set(trimmed);
        val.type_ = Variables::Type::STRING;
        return ValuePtr(val);
    }

    static ValuePtr fromStringToInt(const std::string & str) {
        Value val;
        val.set(std::stoi(str));
        val.type_ = Variables::Type::INTEGER;
        return ValuePtr(val);
    }

    static ValuePtr fromStringToDouble(const std::string & str) {
        Value val;
        val.set(std::stod(str));
        val.type_ = Variables::Type::DOUBLE;
        return ValuePtr(val);
    }

    static ValuePtr fromStringToFloat(const std::string & str) {
        Value val;
        val.set(std::stof(str));
        val.type_ = Variables::Type::FLOAT;
        return ValuePtr(val);
    }

    static ValuePtr fromStringToBool(const std::string & str) {
        std::string s = str;
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        if (s == "true" || s == "1") {
            Value val;
            val.set(true);
            val.type_ = Variables::Type::BOOLEAN;
            return ValuePtr(val);
        }
        if (s == "false" || s == "0") {
            Value val;
            val.set(false);
            val.type_ = Variables::Type::BOOLEAN;
            return ValuePtr(val);
        }
        throw std::invalid_argument("Invalid bool string: " + str);
    }
};

}  // namespace Symbols

#endif
