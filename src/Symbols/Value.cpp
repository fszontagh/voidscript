#include "Symbols/Value.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/VariableTypes.hpp"

#include <algorithm>
#include <limits>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <vector>

// Standard library includes that are likely needed by the moved implementations
#include <algorithm>  // For std::transform in ValuePtr::fromStringToBool
#include <mutex>      // For ValuePtr::registryMutex_
#include <stdexcept>  // For std::bad_cast, std::runtime_error, std::logic_error, std::invalid_argument
#include <string>
#include <vector>     // Potentially used by some methods, good to have

// Note: <map> and <memory> are already included by Value.hpp for ObjectMap and shared_ptr

namespace Symbols {
// --- Implementations for Symbols::Value methods ---

void Value::setNULL() {
    is_null = true;
    data_.reset();
}

Value::Value() {
    setNULL();  // Calls the moved Value::setNULL()
}

void Value::clone_data_from(const Value & other) {
    // Reset current data
    this->data_.reset();
    this->is_null = true;  // Default to null until data is set

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
        case Symbols::Variables::Type::OBJECT:
            {
                const auto & source_map = other.get<ObjectMap>();
                ObjectMap    new_map;
                for (const auto & pair : source_map) {
                    new_map[pair.first] = pair.second.clone();  // Assumes ValuePtr::clone() exists
                }
                this->set<ObjectMap>(new_map);
                break;
            }
        case Symbols::Variables::Type::CLASS:
            {
                const auto & source_map = other.get<ObjectMap>();
                ObjectMap    new_map;
                for (const auto & pair : source_map) {
                    new_map[pair.first] = pair.second.clone();
                }
                this->set<ObjectMap>(new_map);
                this->type_ = Symbols::Variables::Type::CLASS; // Override the type to CLASS
                break;
            }
        case Symbols::Variables::Type::NULL_TYPE:
            this->setNULL();
            break;
        default:
            this->setNULL();
            break;
    }

    if (this->data_) {
        this->is_null = false;
        // type_ and type_id_ are set by set<T>()
    } else {
        // Data was not set (e.g., setNULL() was called or unhandled type).
        this->is_null  = true;
        this->type_    = other.type_;  // Preserve original type info for null values
        this->type_id_ = other.type_id_;
    }
}

std::shared_ptr<Value> Value::clone() const {
    auto new_value = std::make_shared<Value>();

    new_value->type_    = this->type_;
    new_value->type_id_ = this->type_id_;
    new_value->is_null  = this->is_null;

    if (!this->is_null && this->data_) {
        new_value->clone_data_from(*this);
    }
    return new_value;
}

bool Value::isNULL() const {
    // is_null is the primary flag. data_ check is secondary.
    // A Value can be semantically null (is_null = true) but have a type (e.g. null string).
    // A Value might also be is_null = false, but data_ is null if improperly handled (should not happen).
    return is_null || !data_;
}

Symbols::Variables::Type Value::getType() const {
    return type_;
}

std::string Value::toString() const {
    if (isNULL() && type_ != Variables::Type::STRING && type_ != Variables::Type::OBJECT &&
        type_ != Variables::Type::CLASS) {  // Allow toString on null strings/objects
        if (type_ == Variables::Type::NULL_TYPE || (!data_ && type_ != Variables::Type::STRING)) {
            return "null";
        }
    }

    // Specific handling for STRING to allow "null" for null strings vs empty string ""
    if (type_ == Variables::Type::STRING) {
        if (!data_ || is_null) {  // if is_null is true, it's a "null string"
            return "null";
        }
        return get<std::string>();  // Will return "" if string is empty but not null
    }

    // For other types, if data_ is null but is_null wasn't true (e.g. uninitialized non-string)
    // or if is_null is true, then it's "null"
    if (!data_ || is_null) {
        return "null";
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
        case Variables::Type::CLASS: {
            // For CLASS type, return a more descriptive string
            try {
                const auto& objMap = get<ObjectMap>();
                if (objMap.find("__class__") != objMap.end() && objMap.at("__class__").getType() == Variables::Type::STRING) {
                    return "[Class " + objMap.at("__class__").get<std::string>() + "]";
                }
                return "[Class Object]";
            } catch (const std::exception& e) {
                return "[Invalid Class Object]";
            }
        }
        // STRING is handled above
        // OBJECT, NULL_TYPE will fall to default or handled by isNULL checks
        default:
            return "null";  // Should ideally be covered by isNULL or type checks
    }
}

// --- Implementations for Symbols::ValuePtr methods ---

// Private Static Method
std::string ValuePtr::valueToString(const Value & value) {
    // This is a helper and might be less complex than Value::toString()
    // as it's called internally when ptr_ is known to be valid.
    // Or, it could simply call value.toString() if that's preferred.
    // For now, replicating simpler logic:
    if (value.isNULL()) {  // Use Value's own isNULL logic
        return "null";
    }
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
        case Variables::Type::CLASS: {
            // For CLASS type, return a more descriptive string
            try {
                const auto& objMap = value.get<ObjectMap>();
                if (objMap.find("__class__") != objMap.end() && 
                    objMap.at("__class__").getType() == Variables::Type::STRING) {
                    return "[Class " + objMap.at("__class__").get<std::string>() + "]";
                }
                return "[Class Object]";
            } catch (const std::exception& e) {
                return "[Invalid Class Object]";
            }
        }
        default:
            return "null";
    }
}

// Private method
void ValuePtr::ensure_object() {
    if (!ptr_) {
        ptr_ = std::make_shared<Value>();  // Should be initialized to NULL_TYPE by Value constructor
    }
    // If ptr_ points to a Value that is not an OBJECT or CLASS (or is NULL_TYPE),
    // reset it to an empty OBJECT.
    if (ptr_->type_ != Variables::Type::OBJECT && ptr_->type_ != Variables::Type::CLASS) {
        ptr_->set<ObjectMap>({});  // This sets type to OBJECT and data to an empty map
        // ptr_->type_ = Variables::Type::OBJECT; // set<T> already does this
    }
}

ValuePtr ValuePtr::clone() const {
    ValuePtr cloned_value_ptr;
    if (this->ptr_) {
        cloned_value_ptr.ptr_ = this->ptr_->clone();
    }
    // If this->ptr_ is null (should not happen with current constructors),
    // cloned_value_ptr remains default-initialized (ptr_ points to a new Value set to NULL).
    return cloned_value_ptr;
}

Variables::Type ValuePtr::getType() const {
    return ptr_ ? ptr_->type_ : Variables::Type::NULL_TYPE;
}

void ValuePtr::setType(Symbols::Variables::Type type) {
    if (!ptr_) {          // Should not happen with current constructors
        ptr_ = std::make_shared<Value>();
        ptr_->setNULL();  // Ensure it's in a valid null state
    }
    // Allow setting type if current value is conceptually null or uninitialized
    if (ptr_->isNULL() || ptr_->getType() == Variables::Type::NULL_TYPE) {
        ptr_->type_ = type;
        // If it was NULL_TYPE and now becomes e.g. STRING, it's a "null string"
        // If it was already a "null string" (type_ == STRING, is_null == true), this just re-affirms type_
    } else {
        throw std::logic_error("Cannot set type manually on an already initialized, non-null value.");
    }
}

ValuePtr & ValuePtr::setNULL() {
    // ensure_object(); // setNULL is not just for objects. It makes the current ValuePtr null.
    if (!ptr_) {  // Should not happen
        ptr_ = std::make_shared<Value>();
    }
    ptr_->setNULL();
    return *this;
}

ValuePtr::ValuePtr(const char * v) {  // Constructor
    if (v == nullptr) {
        ptr_ = std::make_shared<Value>();
        ptr_->setNULL();
    } else {
        ptr_ = std::make_shared<Value>();
        ptr_->set(std::string(v));
    }
}

// Static methods
ValuePtr ValuePtr::null(Symbols::Variables::Type type) {
    ValuePtr z;  // Default constructor makes ptr_ point to a new Value, which is NULL_TYPE and data_ is null.
    // z->setNULL(); // Already done by Value's default constructor via z's default constructor.
    z.setType(type);  // Set the type of this null value.

                      // Special case for string: a "null string" should perhaps still have valid empty string data
    // if direct get<std::string>() is to be allowed without checking isNULL first.
    // Value::toString() handles isNULL for strings to return "null".
    // Value::clone_data_from relies on get<std::string>() for STRING type.
    // If type is STRING and it's null, get<std::string>() would fail if data_ isn't string.
    // The Value::set<T> method initializes data.
    // If we want a "null string" to be distinguishable from an empty string "" that is not null:
    // - isNULL() must be true.
    // - getType() must be STRING.
    // - A call to get<std::string>() should ideally not throw if it's a "null string".
    // The current Value::setNULL() resets data_.
    // Value::set<std::string>("") would make it a non-null empty string.
    // Let's stick to: a null ValuePtr of type STRING has its type_ = STRING, is_null = true, data_ = nullptr.
    // Value::get<std::string>() will throw if data_ is null. This is consistent.
    // The Value::clone_data_from will call new_value->clone_data_from(*this) only if !this->is_null && this->data_
    // So, if a Value is_null, its data won't be cloned via get<T>(). This is fine.
    // The ValuePtr::null(STRING) method sets the type to STRING. If it represents a null string,
    // its Value object will have type_ = STRING and is_null = true.
    // The original code had: if (type == Variables::Type::STRING) { z->set<std::string>(""); }
    // This makes it a non-null empty string. If we want a "null string", this line should be removed.
    // Given the problem statement, we are refactoring existing code. Let's keep behavior.
    // This means ValuePtr::null(STRING) returns a ValuePtr to an *empty string*, not a *null string*.
    if (type == Variables::Type::STRING) {
        z->set<std::string>("");  // Makes it a non-null empty string
        z->is_null = false;       // Explicitly set after set<std::string>
    } else if (type == Variables::Type::OBJECT) {
        z->set<ObjectMap>({});
        z->is_null = false;
    } else if (type == Variables::Type::CLASS) {
        z->set<ObjectMap>({});
        z->type_ = Variables::Type::CLASS; // Override the type to CLASS
        z->is_null = false;
    }
    // For other types, it remains a "null" value of that type (is_null=true, data_=nullptr)
    // but type_ is set.

    return z;
}

ValuePtr ValuePtr::null() {
    return ValuePtr::null(Variables::Type::NULL_TYPE);
}

// Operators
std::shared_ptr<Value> ValuePtr::operator->() {
    if (!ptr_) {  // Should not happen
        ptr_ = std::make_shared<Value>();
        ptr_->setNULL();
    }
    return ptr_;
}

std::shared_ptr<const Value> ValuePtr::operator->() const {
    if (!ptr_) {
        // This is tricky for const operator. Cannot modify ptr_ if ValuePtr is const.
        // However, ptr_ is mutable std::shared_ptr<Value> ptr_;
        // So, we can initialize it.
        ptr_ = std::make_shared<Value>();
        // ptr_->setNULL(); // Default constructor of Value does this.
    }
    return ptr_;
}

ValuePtr::operator Symbols::Variables::Type() {
    if (!ptr_) {
        return Variables::Type::NULL_TYPE;  // Should not happen
    }
    return ptr_->getType();
}

ValuePtr::operator Symbols::Variables::Type() const {
    if (!ptr_) {
        return Variables::Type::NULL_TYPE;  // Should not happen
    }
    return ptr_->getType();
}

bool ValuePtr::operator==(Symbols::Variables::Type type) const {
    if (!ptr_) {  // Should not happen
        return type == Variables::Type::NULL_TYPE;
    }
    return ptr_->type_ == type;
}

bool ValuePtr::operator!=(Symbols::Variables::Type type) const {
    if (!ptr_) {  // Should not happen
        return type != Variables::Type::NULL_TYPE;
    }
    return ptr_->type_ != type;
}

ValuePtr & ValuePtr::operator[](const std::string & key) {
    ensure_object();                           // Ensures ptr_ is valid and type is OBJECT
    ObjectMap & map = ptr_->get<ObjectMap>();  // get<ObjectMap>() must return a reference
    return map[key];                           // map operator[] default-constructs ValuePtr if key not found
}

// Static method
ValuePtr ValuePtr::makeClassInstance(const ObjectMap & v) {
    // Use the new constructor that takes a bool to indicate it's a class
    auto _class_instance_vp = Symbols::ValuePtr(v, true);
    
    ObjectMap& props = _class_instance_vp->get<ObjectMap>();

    // Ensure all properties are initialized
    if (props.find("__class__") != props.end()) {
        std::string className = props["__class__"]->get<std::string>();
        auto& symbolContainer = Symbols::SymbolContainer::instance();

        // Initialize any missing properties with default values
        if (symbolContainer->hasClass(className)) {
            const auto& classInfo = symbolContainer->getClassInfo(className);
            for (const auto& propInfo : classInfo.properties) {
                std::string instancePropName = propInfo.name;
                // Ensure property name has $ prefix for instance properties
                if (!instancePropName.empty() && instancePropName[0] != '$') {
                    instancePropName.insert(0, 1, '$');
                }
                // Only set if property doesn't exist yet
                if (props.find(instancePropName) == props.end()) {
                    // Create default value for the property
                    switch (propInfo.type) {
                        case Variables::Type::INTEGER:
                            props[instancePropName] = ValuePtr(0);
                            break;
                        case Variables::Type::DOUBLE:
                            props[instancePropName] = ValuePtr(0.0);
                            break;
                        case Variables::Type::FLOAT:
                            props[instancePropName] = ValuePtr(0.0f);
                            break;
                        case Variables::Type::STRING:
                            props[instancePropName] = ValuePtr("");
                            break;
                        case Variables::Type::BOOLEAN:
                            props[instancePropName] = ValuePtr(false);
                            break;
                        case Variables::Type::OBJECT:
                            props[instancePropName] = ValuePtr(ObjectMap());
                            break;
                        default:
                            props[instancePropName] = ValuePtr::null(propInfo.type);
                            break;
                    }
                }
            }
        }
    }



    return _class_instance_vp;
}

std::string ValuePtr::toString() const {
    if (!ptr_) {  // Should not happen
        return "null";
    }
    // Delegate to the Value object's toString method
    return ptr_->toString();
}

// Static parsing methods
ValuePtr ValuePtr::fromString(const std::string & str) {
    if (str == "null") {
        return ValuePtr::null(Variables::Type::NULL_TYPE);
    }
    // Boolean check
    if (str == "true") {
        return ValuePtr(true);
    }
    if (str == "false") {
        return ValuePtr(false);
    }

    // Number check (simplified: check for '.' for double/float, otherwise integer)
    // This is basic, production code might need more robust parsing.
    try {
        if (str.find('.') != std::string::npos) {
            // Potentially double or float. stod can parse both.
            // For simplicity, let's assume double if it has a dot.
            double d = std::stod(str);
            // Check if it's a whole number, could be int, but precision might matter
            // if (d == static_cast<long long>(d)) { /* could be int */ }
            return ValuePtr(d);
        } else {
            long long l = std::stoll(str);  // Use stoll for wider range
            if (l >= std::numeric_limits<int>::min() && l <= std::numeric_limits<int>::max()) {
                return ValuePtr(static_cast<int>(l));
            }
            // If it's too large for int, it could be stored as double or throw error
            // For now, let's assume it fits into double if not int, or just use int if it fits.
            // This part of fromString is tricky without more context on desired behavior for large integers.
            // The original implementation used stoi then stod.
            // Let's try int, then double, then string.
            try {
                size_t pos;
                int    i = std::stoi(str, &pos);
                if (pos == str.length()) {
                    return ValuePtr(i);
                }
            } catch (const std::out_of_range &) {
                // too large for int, try double
            } catch (const std::invalid_argument &) {
                // not an int
            }

            try {
                size_t pos;
                double d = std::stod(str, &pos);
                if (pos == str.length()) {
                    return ValuePtr(d);
                }
            } catch (const std::out_of_range &) {
                // too large for double
            } catch (const std::invalid_argument &) {
                // not a double
            }
        }
    } catch (const std::exception &) {
        // If number parsing fails, fall through to return as string
    }

    // Default to string if not null, bool, or number
    return ValuePtr(str);
}

ValuePtr ValuePtr::fromStringToInt(const std::string & str) {
    try {
        return ValuePtr(std::stoi(str));
    } catch (const std::exception & e) {
        throw std::runtime_error("Failed to convert string to int: '" + str + "'. Error: " + e.what());
    }
}

ValuePtr ValuePtr::fromStringToDouble(const std::string & str) {
    try {
        return ValuePtr(std::stod(str));
    } catch (const std::exception & e) {
        throw std::runtime_error("Failed to convert string to double: '" + str + "'. Error: " + e.what());
    }
}

ValuePtr ValuePtr::fromStringToFloat(const std::string & str) {
    try {
        return ValuePtr(std::stof(str));
    } catch (const std::exception & e) {
        throw std::runtime_error("Failed to convert string to float: '" + str + "'. Error: " + e.what());
    }
}

ValuePtr ValuePtr::fromStringToBool(const std::string & str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "true" || s == "1") {
        return ValuePtr(true);
    }
    if (s == "false" || s == "0") {
        return ValuePtr(false);
    }
    throw std::invalid_argument("Invalid string for bool conversion: " + str);
}

// Static method
ValuePtr ValuePtr::asClass(const ValuePtr & obj) {
    if (obj->getType() != Variables::Type::OBJECT) {
        throw std::runtime_error("Cannot convert non-object to class");
    }
    
    ValuePtr result = obj.clone(); // Clone the object
    result->type_ = Variables::Type::CLASS; // Set type to CLASS
    return result;
}

}  // namespace Symbols
