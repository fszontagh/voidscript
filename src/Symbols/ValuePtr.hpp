#ifndef VALUEPTR_HPP
#define VALUEPTR_HPP

#include <map>
#include <memory>
#include <string>

namespace Symbols {

class Value;

class ValuePtr {
  public:
    using ObjectMap = std::map<std::string, std::shared_ptr<Value>>;

    ValuePtr() = default;

    ValuePtr(std::nullptr_t) : ptr_(nullptr) {}

    ValuePtr(const std::shared_ptr<Value> & ptr) : ptr_(ptr) {}

    operator std::shared_ptr<Value>() const { return ptr_; }

    Value * operator->() { return ptr_.get(); }

    const Value * operator->() const { return ptr_.get(); }

    // access to the shared_ptr itself, if needed
    std::shared_ptr<Value> & raw() { return ptr_; }

    const std::shared_ptr<Value> & raw() const { return ptr_; }

    // assignment from shared_ptr
    ValuePtr & operator=(const std::shared_ptr<Value> & rhs) {
        ptr_ = rhs;
        return *this;
    }

    // assignment from ObjectMap
    ValuePtr & operator=(const ObjectMap & obj) {
        ptr_ = std::make_shared<Value>(obj);
        return *this;
    }

    // equality check with pointer
    friend bool operator==(const ValuePtr & lhs, const std::shared_ptr<Value> & rhs) { return lhs.ptr_ == rhs; }

    friend bool operator!=(const ValuePtr & lhs, const std::shared_ptr<Value> & rhs) { return lhs.ptr_ != rhs; }

    // equality check with nullptr
    friend bool operator==(const ValuePtr & lhs, std::nullptr_t) { return lhs.ptr_ == nullptr; }

    friend bool operator!=(const ValuePtr & lhs, std::nullptr_t) { return lhs.ptr_ != nullptr; }

  private:
    std::shared_ptr<Value> ptr_;
};

}  // namespace Symbols

#endif  // VALUEPTR_HPP
