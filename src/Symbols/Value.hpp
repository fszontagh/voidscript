#ifndef SYMBOLS_VALUE_HPP
#define SYMBOLS_VALUE_HPP

#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>  // For std::bad_cast
#include <string>
#include <typeindex>
#include <typeinfo>

#include "Symbols/VariableTypes.hpp"

namespace Symbols {

class Value;
class ValuePtr;

using ObjectMap = std::map<std::string, ValuePtr>;

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
    friend class ValuePtr;

    void setNULL() {
        is_null = true;
        data_.reset();
    }
  private:
    Symbols::Variables::Type type_ = Variables::Type::NULL_TYPE;
    std::shared_ptr<void>    data_;
    std::type_index          type_id_ = typeid(void);
    bool                     is_null  = false;

    void clone_data_from(const Value& other) {
        // Reset current data
        this->data_.reset();
        this->is_null = true; // Default to null until data is set

        switch (other.type_) {
            case Symbols::Variables::Type::INTEGER:
                this->set<int>(other.get<int>());
                break;
            case Symbols::Variables::Type::DOUBLE:
                this->set<double>(other.get<double>());
                break;
            case Symbols::Variables::Type::FLOAT:
                this->set<float>(other.get<float>());
                break;
            case Symbols::Variables::Type::BOOLEAN:
                this->set<bool>(other.get<bool>());
                break;
            case Symbols::Variables::Type::STRING:
                this->set<std::string>(other.get<std::string>());
                break;
            case Symbols::Variables::Type::OBJECT: {
                const auto& source_map = other.get<ObjectMap>();
                ObjectMap new_map;
                for (const auto& pair : source_map) {
                    // Assuming ValuePtr has a clone() method
                    // new_map[pair.first] = pair.second.clone(); 
                    // For now, let's do a shallow copy for ObjectMap values until ValuePtr::clone() is available
                    // This will be updated in a later step.
                    // The task description says: For each `val_ptr`, call `val_ptr.clone()`
                    // Since ValuePtr::clone() is not yet implemented, this line will cause a compile error.
                    // I will comment it out for now and proceed with the rest of the implementation.
                    // The subtask states: "This clone() method on ValuePtr doesn't exist yet but assume it will"
                    // So I will include the line.
                    new_map[pair.first] = pair.second.clone();
                }
                this->set<ObjectMap>(new_map);
                break;
            }
            case Symbols::Variables::Type::NULL_TYPE:
                this->setNULL();
                break;
            default:
                // Consider throwing an error for unhandled types
                // For now, it will remain null as per the initial reset
                // throw std::runtime_error("Unhandled type in clone_data_from");
                this->setNULL(); // Default to NULL for safety if type is unknown/unhandled
                break;
        }
        // After setting data, update type information from 'other'
        this->type_ = other.type_;
        this->type_id_ = other.type_id_;
        this->is_null = other.is_null; 
        // However, if set<T> was called, is_null would be false.
        // And if setNULL() was called, is_null would be true.
        // The set<T> also sets type_ and type_id_.
        // After the switch, type_, type_id_, and is_null need to be consistent.
        // set<T> sets type_ and type_id_ and implies data_ is not null.
        // setNULL() sets data_ to null and is_null to true.

        if (this->data_) {
            // Data was set by one of the set<T> calls.
            // type_ and type_id_ are already set correctly by set<T>.
            this->is_null = false;
        } else {
            // Data was not set (e.g., setNULL() was called or unhandled type).
            // Ensure is_null is true and copy type info from other.
            this->is_null = true;
            this->type_ = other.type_;
            this->type_id_ = other.type_id_;
        }
    }

    template <typename T> void set(T data) {
        this->data_    = std::make_shared<T>(std::move(data));
        this->type_id_ = std::type_index(typeid(T));
        this->type_    = type_names.at(std::type_index(typeid(T)));  // Get type from predefined map
    }
  public:
    Value() { setNULL(); }

    std::shared_ptr<Value> clone() const {
        auto new_value = std::make_shared<Value>();

        // Copy fundamental type information
        new_value->type_ = this->type_;
        new_value->type_id_ = this->type_id_;
        new_value->is_null = this->is_null;

        // If the current object has data, clone it
        // The check for this->data_ is important because is_null might be true
        // but type_ might not be NULL_TYPE (e.g. an uninitialized string variable)
        if (!this->is_null && this->data_) {
            new_value->clone_data_from(*this);
        }
        // If this->is_null is true, new_value is already in a default null state.
        // Its type_ and type_id_ have been set to match this object,
        // which is correct for representing a null value of a specific type.
        // clone_data_from will handle the case where this->type_ is NULL_TYPE if called.

        return new_value;
    }

    template <typename T> const T & get() const {
        if (type_id_ != typeid(T)) {
            throw std::bad_cast();
        }
        if (!data_) {
            throw std::runtime_error("Attempted to access data, but it's null.");
        }
        return *std::static_pointer_cast<T>(data_);
    }

    bool isNULL() const { return is_null || !data_; }

    template <typename T> T & get() {
        if (type_id_ != typeid(T)) {
            throw std::bad_cast();
        }
        return *std::static_pointer_cast<T>(data_);
    }

    Symbols::Variables::Type getType() const { return type_; }

    std::string toString() const {
        if (isNULL()) {
            return "null";
        }

        if (type_ == Variables::Type::STRING) {
            if (!data_) {
                return "null";  // vagy throw
            }
            return get<std::string>();
        }

        switch (type_) {
            case Variables::Type::INTEGER:
                return std::to_string(get<int>());
            case Variables::Type::FLOAT:
                return std::to_string(get<float>());
            case Variables::Type::DOUBLE:
                return std::to_string(get<double>());
            case Variables::Type::BOOLEAN:
                return get<bool>() ? "true" : "false";
            default:
                return "null";
        }
    }
};

class ValuePtr {
  private:
    mutable std::shared_ptr<Value> ptr_;

    static std::mutex                      registryMutex_;
    static std::map<std::string, ValuePtr> valueRegistry_;

    static std::string valueToString(const Value & value) {
        if (value.type_ == Variables::Type::STRING) {
            return value.get<std::string>();
        }
        switch (value.type_) {
            case Variables::Type::INTEGER:
                return std::to_string(value.get<int>());
            case Variables::Type::FLOAT:
                return std::to_string(value.get<float>());
            case Variables::Type::DOUBLE:
                return std::to_string(value.get<double>());
            case Variables::Type::BOOLEAN:
                return value.get<bool>() ? "true" : "false";
            default:
                return "null";
        }
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

    ValuePtr(const ValuePtr & other) { this->ptr_ = other.ptr_; }

    ValuePtr & operator=(const ValuePtr & other) {
        ptr_ = other.ptr_;
        return *this;
    }

    ValuePtr & operator=(ValuePtr && other) noexcept {
        ptr_ = std::move(other.ptr_);
        return *this;
    }

    ValuePtr(int v) : ptr_(std::make_shared<Value>()) { ptr_->set(v); }

    ValuePtr(float v) : ptr_(std::make_shared<Value>()) { ptr_->set(v); }

    ValuePtr(double v) : ptr_(std::make_shared<Value>()) { ptr_->set(v); }

    ValuePtr(bool v) : ptr_(std::make_shared<Value>()) { ptr_->set(v); }

    ValuePtr(const std::string & v) : ptr_(std::make_shared<Value>()) { ptr_->set(v); }

    /*
    ValuePtr(const char * v) {
        if ()
        ptr_ = std::make_shared<Value>();
        ptr_->set(std::string(v));
        ptr_->type_ = Variables::Type::STRING;
    }
*/
    ValuePtr(const ObjectMap & v) : ptr_(std::make_shared<Value>()) { ptr_->set(v); }

    ValuePtr clone() const {
        ValuePtr cloned_value_ptr; // Default-constructs: ptr_ points to a new Value, set to NULL.
        if (this->ptr_) {
            // If the current ValuePtr actually points to a Value object
            // (even if that Value object represents null type like an uninitialized variable)
            // then clone that Value object.
            // Value::clone() returns std::shared_ptr<Value>
            cloned_value_ptr.ptr_ = this->ptr_->clone();
        }
        // If this->ptr_ was itself a nullptr (which current ValuePtr constructors prevent,
        // as they always initialize ptr_ to a valid std::shared_ptr<Value>),
        // or if Value::clone() could return nullptr (it is designed to always return a valid shared_ptr),
        // then cloned_value_ptr remains in its default initialized state (a valid ValuePtr holding a Value in NULL state).
        return cloned_value_ptr;
    }

    ValuePtr(Value && v) {
        if (!ptr_) {
            Symbols::Variables::Type type = v.getType();                            // Capture type before move
            ptr_                          = std::make_shared<Value>(std::move(v));  // Proper move semantics
            ptr_->type_                   = type;  // Assign captured type to new instance
        }
    }

    Variables::Type getType() const { return ptr_ ? ptr_->type_ : Variables::Type::NULL_TYPE; }

    void setType(Symbols::Variables::Type type) {
        if (!ptr_ || ptr_->isNULL()) {
            ptr_->type_ = type;
        } else {
            throw std::logic_error("Cannot set type manually on non-null value");
        }
    }

    ValuePtr & setNULL() {
        ensure_object();
        ptr_->setNULL();
        return *this;
    }

    ValuePtr(const char * v) {
        if (v == nullptr) {
            ptr_ = std::make_shared<Value>();
            ptr_->setNULL();
        } else {
            ptr_ = std::make_shared<Value>();
            ptr_->set(std::string(v));
        }
    }

    static ValuePtr null(Symbols::Variables::Type type) {
        auto z = ValuePtr(nullptr);
        z->setNULL();  // <- data_ reset
        z.setType(type);
        if (type == Variables::Type::STRING) {
            z->set<std::string>("");
        }
        return z;
    }

    static ValuePtr null() { return ValuePtr::null(Variables::Type::NULL_TYPE); }

    std::shared_ptr<Value> operator->() {
        if (!ptr_) {
            ptr_ = std::make_shared<Value>();
            ptr_->setNULL();
        }
        return ptr_;
    }

    std::shared_ptr<const Value> operator->() const {
        if (!ptr_) {
            ptr_ = std::make_shared<Value>();
        }
        return ptr_;
    }

    template <typename T> T & get() { return ptr_->get<T>(); }

    template <typename T> const T & get() const { return ptr_->get<T>(); }

    operator Symbols::Variables::Type() { return ptr_->getType(); }

    operator Symbols::Variables::Type() const {
        if (!ptr_) {
            return Variables::Type::NULL_TYPE;
        }
        return ptr_->getType();
    }

    bool operator==(Symbols::Variables::Type type) const {
        if (!ptr_) {
            return type == Variables::Type::NULL_TYPE;
        }
        return ptr_->type_ == type;
    }

    bool operator!=(Symbols::Variables::Type type) const {
        if (!ptr_) {
            return type != Variables::Type::NULL_TYPE;
        }
        return ptr_->type_ != type;
    }

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
        if (ptr_->type_ == Variables::Type::STRING) {
            if (!ptr_->data_) {
                return "null";
            }
            return ptr_->get<std::string>();
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
            case Variables::Type::OBJECT:
                return "{...}";
            case Variables::Type::NULL_TYPE:
            default:
                return "null";
        }
    }

    static ValuePtr fromString(const std::string & str) {
        if (str == "null") {
            return ValuePtr::null(Variables::Type::NULL_TYPE);
        }
        try {
            // Try boolean first
            if (str == "true") {
                return ValuePtr(true);
            }
            if (str == "false") {
                return ValuePtr(false);
            }

            // Try numbers
            if (str.find('.') != std::string::npos) {
                double d = std::stod(str);
                return ValuePtr(d);
            }
            int i = std::stoi(str);
            return ValuePtr(i);
        } catch (const std::exception & e) {
            // Handle invalid string
            return ValuePtr(str);
        }
    }

    static ValuePtr fromStringToInt(const std::string & str) { return ValuePtr(std::stoi(str)); }

    static ValuePtr fromStringToDouble(const std::string & str) { return ValuePtr(std::stod(str)); }

    static ValuePtr fromStringToFloat(const std::string & str) { return ValuePtr(std::stof(str)); }

    static ValuePtr fromStringToBool(const std::string & str) {
        std::string s = str;
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        if (s == "true" || s == "1") {
            return ValuePtr(true);
        }
        if (s == "false" || s == "0") {
            return ValuePtr(false);
        }
        throw std::invalid_argument("Invalid bool string: " + str);
    }
};

}  // namespace Symbols

#endif
