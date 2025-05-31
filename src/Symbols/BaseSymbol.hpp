// BaseSymbol.hpp
#ifndef BASE_SYMBOL_HPP
#define BASE_SYMBOL_HPP

#include <iostream> // For std::cerr, std::endl
#include <string>
#include <utility>

#include "SymbolKind.hpp"
#include "Symbols/VariableTypes.hpp"
#include "Value.hpp"

namespace Symbols {

class Symbol {
  protected:
    std::string   name_;
    ValuePtr      value_;
    std::string   context_;  // ns
    Symbols::Kind kind_;

  public:
    Symbol(const std::string & name, ValuePtr value, const std::string & context, Symbols::Kind type) :
        name_(name),
        value_(value),
        context_(context),
        kind_(type) {}

    virtual ~Symbol() = default;

    virtual Symbols::Kind kind() const = 0;

    const std::string & name() const { return name_; }

    const std::string & context() const { return context_; }

    Symbols::Kind getKind() const { return kind_; }

    virtual const ValuePtr & getValue() const {
        // +++ Add New Logging +++
        // +++ Refined Logging for getValue +++
        std::cerr << "[DEBUG SYMBOL_GET_VALUE] Symbol: '" << name_ << "' (Context: " << context_
                  << ", Kind: " << static_cast<int>(kind_) << ") - getValue() called." << std::endl;
        if (!value_.operator->()) {
            std::cerr << "[DEBUG SYMBOL_GET_VALUE]   INTERNAL ERROR: value_.ptr_ is C++ nullptr for symbol '" << name_ << "'!" << std::endl;
        } else {
            std::cerr << "[DEBUG SYMBOL_GET_VALUE]   About to return value_.toString(): " << value_.toString()
                      << ". Direct check of value_.ptr_->is_null: " << (value_->is_null ? "true" : "false")
                      << ". value_->getType(): " << Symbols::Variables::TypeToString(value_->getType()) << std::endl;
        }
        // +++ End Refined Logging +++
        return value_;
    }

    //virtual const Value & getValue() const { return value_; }

    virtual void setValue(const ValuePtr & value) {
        // +++ Add New Logging for setValue +++
        std::cerr << "[DEBUG SYMBOL_SET_VALUE] Symbol: '" << name_ << "' (Context: " << context_ << ", Kind: " << static_cast<int>(kind_) << ")"
                  << ", setValue() called. New Value: " << value.toString();
        if (value.operator->()) {
             std::cerr << " (New Value's Value::is_null actual flag: " << (value->is_null ? "true" : "false") << ")";
        } else {
            std::cerr << " (New ValuePtr's internal shared_ptr is C++ nullptr!)";
        }
        std::cerr << std::endl;
        // +++ End New Logging for setValue +++
        value_ = value;
    }

    // Dump symbol details (default: type and value)
    virtual std::string dump() const {
        std::string r = "\t\t  " + kindToString(this->kind_) + " name: '" + name_ + "' \n\t\t\tContext: " + context_;
        r += " \n\t\t\tType: " + Symbols::Variables::TypeToString(value_.getType());
        r += " \n\t\t\tValue: '" + value_.toString() + "'";
        return r;
    }

    // Templated getter
    template <typename T> T getAs() const { return std::get<T>(value_); }
};

}  // namespace Symbols

#endif
