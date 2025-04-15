// BaseSymbol.hpp
#ifndef BASE_SYMBOL_HPP
#define BASE_SYMBOL_HPP

#include <string>

#include "SymbolKind.hpp"
#include "Value.hpp"

namespace Symbols {

class Symbol {
  protected:
    std::string   name_;
    Value         value_;
    std::string   context_;
    Symbols::Kind kind_;

  public:
    Symbol(const std::string & name, const Value & value, const std::string & context, Symbols::Kind type) :
        name_(name),
        value_(value),
        context_(context),
        kind_(type) {}

    virtual ~Symbol() = default;

    // Polimorf azonosító
    virtual Symbols::Kind kind() const = 0;

    // Getterek
    const std::string & name() const { return name_; }

    const std::string & context() const { return context_; }

    Symbols::Kind type() const { return kind_; }

    // Virtuális getter/setter a value-hoz
    virtual const Value & getValue() const { return value_; }

    virtual void setValue(const Value & value) { value_ = value; }

    // Templated getter
    template <typename T> T getAs() const { return std::get<T>(value_); }
};

}  // namespace Symbols

#endif
