#ifndef ValueP_HPP
#define ValueP_HPP

#include <map>
#include <memory>
#include <string>
#include <variant>

class Value;

namespace Symbols {
class ValueP {
  public:
    using ObjectMap = std::map<std::string, std::shared_ptr<Value>>;

    ValueP() = default;

    ValueP(std::nullptr_t) : ptr_(nullptr) {}

    ValueP(const std::shared_ptr<Value> & ptr) : ptr_(ptr) {}

    // static factory methods
    static ValueP create() { return std::make_shared<Value>(); }

    template <typename T> static ValueP create(const T & val) { return std::make_shared<Value>(val); }

    static ValueP createObjectMap() { return create(ObjectMap{}); }

    static ValueP createObjectMap(const ObjectMap & val) { return create(val); }

    Value * operator->() { return ptr_.get(); }

    const Value * operator->() const { return ptr_.get(); }

    std::shared_ptr<Value> & raw() { return ptr_; }

    const std::shared_ptr<Value> & raw() const { return ptr_; }

    ValueP & operator=(const std::shared_ptr<Value> & rhs) {
        ptr_ = rhs;
        return *this;
    }

    ValueP & operator=(const ObjectMap & obj) {
        ptr_ = std::make_shared<Value>(obj);
        return *this;
    }

    operator std::shared_ptr<Value>() const { return ptr_; }

    operator ObjectMap() const {
        if (!ptr_ || !std::holds_alternative<ObjectMap>(*ptr_->get())) {
            throw std::invalid_argument("Value does not contain ObjectMap");
        }
        return std::get<ObjectMap>(*ptr_->get());
    }

  private:
    std::shared_ptr<Value> ptr_;
};

}  // namespace Symbols

#endif  // ValueP_HPP
