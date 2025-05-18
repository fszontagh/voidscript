// VariableSymbol.hpp
#ifndef VARIABLE_SYMBOL_HPP
#define VARIABLE_SYMBOL_HPP

#include "BaseSymbol.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Symbols {

class VariableSymbol : public Symbol {
  protected:
    Symbols::Variables::Type type_;
  public:
    VariableSymbol(const std::string & name, Value::ValuePtr & value, const std::string & context,
                   Variables::Type type) :
        Symbols::Symbol(name, std::move(value), context, Symbols::Kind::Variable),
        type_(type) {}

    Symbols::Kind kind() const override { return Symbols::Kind::Variable; }

    Variables::Type type() const { return type_; }

    std::string toString() const {
        std::string r = "VariableSymbol: " + name_ + " Type: " + Symbols::Variables::TypeToString(type_);
        if (type_ == Symbols::Variables::Type::INTEGER) {
            r += " Value: " + std::to_string(value_->get<int>());
        } else if (type_ == Symbols::Variables::Type::DOUBLE) {
            r += " Value: " + std::to_string(value_->get<double>());
        } else if (type_ == Symbols::Variables::Type::FLOAT) {
            r += " Value: " + std::to_string(value_->get<float>());
        } else if (type_ == Symbols::Variables::Type::STRING) {
            r += " Value: " + value_->get<std::string>();
        } else if (type_ == Symbols::Variables::Type::BOOLEAN) {
            r += " Value: " + std::to_string(value_->get<bool>());
        } else if (type_ == Symbols::Variables::Type::NULL_TYPE) {
            r += " Value: null";
        }

        return r;
    }
};

}  // namespace Symbols

#endif
