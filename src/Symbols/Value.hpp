#ifndef SYMBOLS_VALUE_HPP
#define SYMBOLS_VALUE_HPP

#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "Symbols/VariableTypes.hpp"
#include "VariableTypes.hpp"

namespace Symbols {

class Value;
class ValuePtr;

using ObjectMap = std::map<std::string, ValuePtr>;

// Type mapping
const static std::unordered_map<std::type_index, Symbols::Variables::Type> type_names = {
    { std::type_index(typeid(int)),         Symbols::Variables::Type::INTEGER   },
    { std::type_index(typeid(double)),      Symbols::Variables::Type::DOUBLE    },
    { std::type_index(typeid(float)),       Symbols::Variables::Type::FLOAT     },
    { std::type_index(typeid(bool)),        Symbols::Variables::Type::BOOLEAN   },
    { std::type_index(typeid(ObjectMap)),   Symbols::Variables::Type::OBJECT    },
    { std::type_index(typeid(std::string)), Symbols::Variables::Type::STRING    },
    { std::type_index(typeid(nullptr)),     Symbols::Variables::Type::NULL_TYPE },
};

class Value {
    friend class ValuePtr;  // ValuePtr needs access to Value's private members

  private:
    Symbols::Variables::Type type_ = Variables::Type::NULL_TYPE;
    std::shared_ptr<void>    data_;
    std::type_index          type_id_ = typeid(void);
  public: // Temporarily public for debugging
    bool                     is_null  = false;
  private: // Back to private

    // Private methods - Declarations only
    void setNULL();
    void clone_data_from(const Value & other);

    // Templated methods remain in the header
    template <typename T> void set(T data) {
        this->data_    = std::make_shared<T>(std::move(data));
        this->type_id_ = std::type_index(typeid(T));
        // Safely access type_names, ensuring the type exists
        auto it        = type_names.find(std::type_index(typeid(T)));
        if (it != type_names.end()) {
            this->type_ = it->second;
        } else {
            // Handle unknown type, perhaps throw or set to a default/unknown type
            // For now, let's assume types used with set<T> are in type_names
            // or this could be an assertion point depending on design philosophy.
            throw std::runtime_error("Type not found in type_names map during Value::set");
        }
        this->is_null = false;  // Successfully setting data means it's not null.
    }

  public:
    // Constructor - Declaration only
    Value();

    // Public methods - Declarations only
    std::shared_ptr<Value>   clone() const;
    bool                     isNULL() const;
    Symbols::Variables::Type getType() const;
    std::string              toString() const;

    std::string getDebugStateString() const {
        std::string type_str       = Symbols::Variables::TypeToString(this->type_);
        std::string null_str       = this->is_null ? "true" : "false";
        std::string data_valid_str = this->data_ ? "true" : "false";
        return "type='" + type_str + "', is_null='" + null_str + "', data_ptr_valid='" + data_valid_str + "'";
    }

    // Templated methods remain in the header
    template <typename T> const T & get() const {
        if (type_id_ != typeid(T)) {
            std::string expected_name = Symbols::Variables::TypeToString(type_names.at(typeid(T)));
            std::string got_name      = Symbols::Variables::TypeToString(type_names.at(type_id_));
            std::string error_msg     = "Bad cast, expected: " + expected_name + " got: " + got_name;
            throw std::runtime_error(error_msg);
        }
        if (!data_) {  // Check if data pointer is null
            std::string type_str = Symbols::Variables::TypeToString(this->type_);
            std::string err_msg =
                "Attempted to access data from a Value object with null data pointer. Value state: type='" + type_str +
                "', is_null='" + (this->is_null ? "true" : "false") + "'.";
            throw std::runtime_error(err_msg);
        }
        return *std::static_pointer_cast<T>(data_);
    }

    template <typename T> T & get() {
        if (type_id_ != typeid(T)) {
            std::string expected_name = Symbols::Variables::TypeToString(type_names.at(typeid(T)));
            std::string got_name      = Symbols::Variables::TypeToString(type_names.at(type_id_));
            std::string error_msg     = "Bad cast, expected: " + expected_name + " got: " + got_name;
            throw std::runtime_error(error_msg);
        }
        if (!data_) {  // Check if data pointer is null
            std::string type_str = Symbols::Variables::TypeToString(this->type_);
            std::string err_msg =
                "Attempted to access data from a Value object with null data pointer. Value state: type='" + type_str +
                "', is_null='" + (this->is_null ? "true" : "false") + "'.";
            throw std::runtime_error(err_msg);
        }
        return *std::static_pointer_cast<T>(data_);
    }
};

class ValuePtr {
  private:
    mutable std::shared_ptr<Value> ptr_;  // Mutable to allow lazy initialization in const operator->
    // Private methods - Declarations only
    static std::string             valueToString(const Value & value);
    void                           ensure_object();

  public:
    // Default constructor: Inlined as it's simple and commonly used.
    // Initializes ptr_ to a new Value, which itself initializes to NULL_TYPE.
    ValuePtr() : ptr_(std::make_shared<Value>()) {
        // Value's default constructor calls its setNULL(), so ptr_ points to a null Value.
    }

    // Copy constructor: Inlined, standard shared_ptr copy.
    ValuePtr(const ValuePtr & other) : ptr_(other.ptr_) {}

    // Move constructor: Inlined, standard shared_ptr move.
    ValuePtr(ValuePtr && other) noexcept : ptr_(std::move(other.ptr_)) {}

    // Assignment operators: Inlined.
    ValuePtr & operator=(const ValuePtr & other) {
        if (this != &other) {
            ptr_ = other.ptr_;
        }
        return *this;
    }

    ValuePtr & operator=(ValuePtr && other) noexcept {
        if (this != &other) {
            ptr_ = std::move(other.ptr_);
        }
        return *this;
    }

    // Constructors for primitive types: Inlined for efficiency.
    ValuePtr(int v) : ptr_(std::make_shared<Value>()) { ptr_->set(v); }

    ValuePtr(float v) : ptr_(std::make_shared<Value>()) { ptr_->set(v); }

    ValuePtr(double v) : ptr_(std::make_shared<Value>()) { ptr_->set(v); }

    ValuePtr(bool v) : ptr_(std::make_shared<Value>()) { ptr_->set(v); }

    ValuePtr(const std::string & v) : ptr_(std::make_shared<Value>()) { ptr_->set(v); }

    ValuePtr(const ObjectMap & v) : ptr_(std::make_shared<Value>()) { ptr_->set(v); }

    // Constructor for class types
    ValuePtr(const ObjectMap & v, bool isClass) : ptr_(std::make_shared<Value>()) {
        ptr_->set(v);
        if (isClass) {
            ptr_->type_ = Variables::Type::CLASS;
        }
    }

    // Constructor from Value&& : Inlined. Captures type correctly.
    ValuePtr(Value && v) : ptr_(std::make_shared<Value>(std::move(v))) {
        // The moved-from 'v' is in an unspecified state.
        // The new Value object created from 'v' should have its type correctly set
        // by Value's move constructor if it exists, or this needs careful handling.
        // Assuming Value's internals (type_, type_id_, is_null, data_) are correctly transferred.
        // ptr_->type_ = v.getType(); // This would be problematic as v is moved from.
        // The type should be inherent in the moved Value object.
    }

    // Constructor from const char* - Declaration only (moved to .cpp)
    ValuePtr(const char * v);

    // Public methods - Declarations only
    ValuePtr        clone() const;
    Variables::Type getType() const;
    void            setType(Symbols::Variables::Type type);
    ValuePtr &      setNULL();

    // Static methods - Declarations only
    static ValuePtr null(Symbols::Variables::Type type);
    static ValuePtr null();
    static ValuePtr undefined();
    static ValuePtr makeClassInstance(const ObjectMap & v);
    static ValuePtr fromString(const std::string & str);
    static ValuePtr fromStringToInt(const std::string & str);
    static ValuePtr fromStringToDouble(const std::string & str);
    static ValuePtr fromStringToFloat(const std::string & str);
    static ValuePtr fromStringToBool(const std::string & str);

    // Convert an object to a class type
    static ValuePtr asClass(const ValuePtr & obj);

    // Operators - Declarations for those moved, inline for trivial ones
    std::shared_ptr<Value>       operator->();
    std::shared_ptr<const Value> operator->() const;

    // Type conversion operators - Declarations only
    operator Symbols::Variables::Type();        // Made implicit
    operator Symbols::Variables::Type() const;  // Made implicit

    // Comparison operators - Declarations only
    bool operator==(Symbols::Variables::Type type) const;
    bool operator!=(Symbols::Variables::Type type) const;

    // Subscript operator for objects - Declaration only
    ValuePtr & operator[](const std::string & key);

    ValuePtr & operator[](const char * key) {
        return operator[](std::string(key));  // Reuse the string version
    }

    // toString method - For debug/printing
    std::string toString() const;

    // Template conversion operator for all types
    // Templated getter methods
    template <typename T> T & get() {
        if (!ptr_) {
            throw std::runtime_error("ValuePtr has null internal pointer in get<T>().");
        }
        return ptr_->get<T>();
    }

    template <typename T> const T & get() const {
        if (!ptr_) {
            throw std::runtime_error("ValuePtr has null internal pointer in const get<T>().");
        }
        return ptr_->get<T>();
    }

    // Universal conversion operator with type constraints
    template <typename T>
    requires(std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_same_v<T, double> ||
             std::is_same_v<T, ObjectMap> || std::is_same_v<T, bool> ||
             std::is_same_v<T, std::string>) operator T() const {
        if (!ptr_) {
            throw std::runtime_error("Cannot convert null ValuePtr (universal conversion operator)");
        }
        if (ptr_->isNULL()) {
            throw std::runtime_error("Cannot convert NULL value (universal conversion operator)");
        }

        if constexpr (std::is_same_v<T, bool>) {
            // For boolean conversion, handle numeric types and comparison results
            switch (ptr_->getType()) {
                case Variables::Type::BOOLEAN:
                    return ptr_->get<bool>();
                case Variables::Type::INTEGER:
                    return ptr_->get<int>() != 0;
                case Variables::Type::FLOAT:
                    return ptr_->get<float>() != 0;
                case Variables::Type::DOUBLE:
                    return ptr_->get<double>() != 0;
                case Variables::Type::STRING:
                    return !ptr_->get<std::string>().empty();
                case Variables::Type::OBJECT:
                case Variables::Type::CLASS:
                    // For objects and classes, treat as boolean
                    try {
                        // Try to get as bool first
                        return ptr_->get<bool>();
                    } catch (const std::runtime_error &) {
                        // If not a direct bool, assume non-empty object is true
                        return !ptr_->get<ObjectMap>().empty();
                    }
                default:
                    throw std::runtime_error("Bad cast, cannot convert type " +
                                             std::to_string(static_cast<int>(ptr_->getType())) + " to bool");
            }
        }

        return ptr_->get<T>();
    }

    // Specialized boolean conversion operator
    operator bool() const {
        if (!ptr_) {
            throw std::runtime_error("Cannot convert null ValuePtr (bool operator)");
        }
        if (ptr_->isNULL()) {
            throw std::runtime_error("Cannot convert NULL value (bool operator)");
        }

        switch (ptr_->getType()) {
            case Variables::Type::BOOLEAN:
                return ptr_->get<bool>();
            case Variables::Type::INTEGER:
                return ptr_->get<int>() != 0;
            case Variables::Type::FLOAT:
                return ptr_->get<float>() != 0.0f;
            case Variables::Type::DOUBLE:
                return ptr_->get<double>() != 0.0;
            case Variables::Type::STRING:
                return !ptr_->get<std::string>().empty();
            case Variables::Type::OBJECT:
            case Variables::Type::CLASS:
                try {
                    return ptr_->get<bool>();
                } catch (const std::runtime_error &) {
                    return !ptr_->get<ObjectMap>().empty();
                }
            default:
                throw std::runtime_error("Cannot convert type to boolean");
        }
    }
};

}  // namespace Symbols

#endif  // SYMBOLS_VALUE_HPP
