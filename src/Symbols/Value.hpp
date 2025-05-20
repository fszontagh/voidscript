#ifndef SYMBOLS_VALUE_HPP
#define SYMBOLS_VALUE_HPP

#include <algorithm> // For ValuePtr::fromStringToBool, kept if any part of it remains in header
#include <map>
#include <memory>
#include <stdexcept>  // For std::bad_cast, kept for get<T>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector> // Potentially for future use or if a moved method implicitly needed it via another header method

#include "Symbols/VariableTypes.hpp" // Essential

namespace Symbols {

class Value; // Forward declaration
class ValuePtr; // Forward declaration

using ObjectMap = std::map<std::string, ValuePtr>;

// This map is used by Value::set<T> which is templated and remains in the header.
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
    friend class ValuePtr; // ValuePtr needs access to Value's private members

  private:
    Symbols::Variables::Type type_ = Variables::Type::NULL_TYPE;
    std::shared_ptr<void>    data_;
    std::type_index          type_id_ = typeid(void);
    bool                     is_null  = false;

    // Private methods - Declarations only
    void setNULL();
    void clone_data_from(const Value& other);

    // Templated methods remain in the header
    template <typename T> void set(T data) {
        this->data_    = std::make_shared<T>(std::move(data));
        this->type_id_ = std::type_index(typeid(T));
        // Safely access type_names, ensuring the type exists
        auto it = type_names.find(std::type_index(typeid(T)));
        if (it != type_names.end()) {
            this->type_ = it->second;
        } else {
            // Handle unknown type, perhaps throw or set to a default/unknown type
            // For now, let's assume types used with set<T> are in type_names
            // or this could be an assertion point depending on design philosophy.
            throw std::runtime_error("Type not found in type_names map during Value::set");
        }
        this->is_null = false; // Successfully setting data means it's not null.
    }

  public:
    // Constructor - Declaration only
    Value();

    // Public methods - Declarations only
    std::shared_ptr<Value> clone() const;
    bool isNULL() const;
    Symbols::Variables::Type getType() const;
    std::string toString() const;

    // Templated methods remain in the header
    template <typename T> const T & get() const {
        if (type_id_ != typeid(T)) {
            // Consider logging or more detailed error message
            // E.g. "Bad cast: expected " + demangled_name(typeid(T)) + " got " + demangled_name(type_id_)
            throw std::bad_cast();
        }
        if (!data_) { // Check if data pointer is null
            throw std::runtime_error("Attempted to access data from a Value object with null data pointer.");
        }
        return *std::static_pointer_cast<T>(data_);
    }

    template <typename T> T & get() {
        if (type_id_ != typeid(T)) {
            throw std::bad_cast();
        }
        if (!data_) {
            throw std::runtime_error("Attempted to access data from a Value object with null data pointer (non-const get).");
        }
        return *std::static_pointer_cast<T>(data_);
    }
};

class ValuePtr {
  private:
    mutable std::shared_ptr<Value> ptr_; // Mutable to allow lazy initialization in const operator->

    // Static members - Declarations only
    static std::mutex                      registryMutex_;
    static std::map<std::string, ValuePtr> valueRegistry_;

    // Private methods - Declarations only
    static std::string valueToString(const Value & value);
    void ensure_object();

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
    
    // Constructor from Value&& : Inlined. Captures type correctly.
    ValuePtr(Value && v) : ptr_(std::make_shared<Value>(std::move(v))) {
        // The moved-from 'v' is in an unspecified state.
        // The new Value object created from 'v' should have its type correctly set
        // by Value's move constructor if it exists, or this needs careful handling.
        // Assuming Value's internals (type_, type_id_, is_null, data_) are correctly transferred.
        // If Value's move constructor doesn't exist or fully set these, this might be an issue.
        // For now, assume Value(std::move(v)) correctly sets up the new Value.
        // ptr_->type_ = v.getType(); // This would be problematic as v is moved from.
        // The type should be inherent in the moved Value object.
    }

    // Constructor from const char* - Declaration only (moved to .cpp)
    ValuePtr(const char * v);


    // Public methods - Declarations only
    ValuePtr clone() const;
    Variables::Type getType() const;
    void setType(Symbols::Variables::Type type);
    ValuePtr & setNULL();

    // Static methods - Declarations only
    static ValuePtr null(Symbols::Variables::Type type);
    static ValuePtr null();
    static ValuePtr makeClassInstance(const ObjectMap & v);
    static ValuePtr fromString(const std::string & str);
    static ValuePtr fromStringToInt(const std::string & str);
    static ValuePtr fromStringToDouble(const std::string & str);
    static ValuePtr fromStringToFloat(const std::string & str);
    static ValuePtr fromStringToBool(const std::string & str);
    
    // Operators - Declarations for those moved, inline for trivial ones
    std::shared_ptr<Value> operator->();
    std::shared_ptr<const Value> operator->() const;

    // Type conversion operators - Declarations only
    explicit operator Symbols::Variables::Type(); // Made explicit to avoid unintentional conversions
    explicit operator Symbols::Variables::Type() const; // Made explicit

    // Comparison operators - Declarations only
    bool operator==(Symbols::Variables::Type type) const;
    bool operator!=(Symbols::Variables::Type type) const;
    
    // Subscript operator for objects - Declaration only
    ValuePtr & operator[](const std::string & key);

    // toString method - Declaration only
    std::string toString() const;

    // Templated getter methods remain in the header
    template <typename T> T & get() {
        if (!ptr_) throw std::runtime_error("ValuePtr has null internal pointer in get<T>().");
        return ptr_->get<T>();
    }

    template <typename T> const T & get() const {
        if (!ptr_) throw std::runtime_error("ValuePtr has null internal pointer in const get<T>().");
        return ptr_->get<T>();
    }

    // Templated conversion operator: Remains in header.
    // Added requires clause for SFINAE, assuming C++20.
    template <typename T>
    requires(std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_same_v<T, double> ||
             std::is_same_v<T, ObjectMap> || std::is_same_v<T, bool> ||
             std::is_same_v<T, std::string>)
    operator T() const {
        if (!ptr_) {
            // This behavior might need refinement. What should converting a null ValuePtr return?
            // For primitives, it might be 0/false/"". For ObjectMap, an empty map.
            // Throwing an exception is safer to indicate an unexpected state.
            throw std::runtime_error("Attempted to convert a ValuePtr with a null internal pointer.");
        }
        // Ensure that if ptr_ points to a NULL Value, get<T> handles it (throws or returns default for T)
        // Current Value::get<T> throws if data_ is null.
        if (ptr_->isNULL()) {
             // Handle conversion from a semantically NULL Value.
             // This is tricky. For example, if T is int, should it be 0? If string, ""?
             // Throwing might be the most consistent with Value::get behavior.
             // Or, provide default values. For now, let get<T> handle it.
             // This path will likely throw if ptr_->data_ is null.
        }
        return ptr_->get<T>();
    }
};

}  // namespace Symbols

#endif // SYMBOLS_VALUE_HPP
