#ifndef SYMBOL_HPP
#define SYMBOL_HPP

#include <string>

#include "Value.hpp"

namespace Symbols {

enum class Kind { Variable, Constant, Function, Unknown };

// Helper function to convert Kind enum to string for debugging
inline std::string kindToString(Kind kind) {
    switch (kind) {
        case Kind::Variable: return "Variable";
        case Kind::Constant: return "Constant";
        case Kind::Function: return "Function";
        case Kind::Unknown: return "Unknown";
        default: return "Invalid";
    }
}

class Symbol {
  private:
    std::string name_;
    std::string context_;
    Value       value_;

  public:
    Symbol(const std::string & name, const Value & value, const std::string & context) :
        name_(name), context_(context), value_(value) {}

    virtual ~Symbol() = default;

    std::string name() const { return name_; }
    std::string context() const { return context_; }
    Value       getValue() const { return value_; }
    void        setValue(const Value & value) { value_ = value; }

    virtual Kind getKind() const { return Kind::Unknown; }
};

using SymbolPtr = std::shared_ptr<Symbol>;

}  // namespace Symbols

#endif 