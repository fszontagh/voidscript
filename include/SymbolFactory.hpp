// SymbolFactory.hpp
#ifndef SYMBOL_FACTORY_HPP
#define SYMBOL_FACTORY_HPP

#include <memory>
#include <string>

#include "ConstantSymbol.hpp"
#include "FunctionSymbol.hpp"
#include "VariableSymbol.hpp"

namespace Symbols {

class SymbolFactory {
  public:
    static std::shared_ptr<Symbol> createVariable(const std::string & name, const Symbols::Value & value,
                                                  const std::string & context, Variables::Type type) {
        return std::make_shared<VariableSymbol>(name, value, context, type);
    }

    static std::shared_ptr<Symbol> createConstant(const std::string & name, const Symbols::Value & value,
                                                  const std::string & context) {
        return std::make_shared<ConstantSymbol>(name, value, context);
    }

    static std::shared_ptr<Symbol> createFunction(const std::string & name, const std::string & context,
                                                  const Symbols::ValueContainer & parameters = {}) {
        return std::make_shared<FunctionSymbol>(name, context, parameters);
    }

    static std::shared_ptr<Symbol> createFunction(const std::string & name, const std::string & context,
                                                  const Symbols::ValueContainer & parameters,
                                                  const std::string &             plainBody) {
        return std::make_shared<FunctionSymbol>(name, context, parameters, plainBody);
    }

    // Overloadok
    static std::shared_ptr<Symbol> createVariable(const std::string & name, int value, const std::string & context) {
        return createVariable(name, Symbols::Value(value), context, Symbols::Variables::Type::VT_INT);
    }

    static std::shared_ptr<Symbol> createVariable(const std::string & name, double value, const std::string & context) {
        return createVariable(name, Symbols::Value(value), context, Symbols::Variables::Type::VT_DOUBLE);
    }

    static std::shared_ptr<Symbol> createVariable(const std::string & name, const std::string & value,
                                                  const std::string & context) {
        return createVariable(name, Symbols::Value(value), context, Symbols::Variables::Type::VT_STRING);
    }

    static std::shared_ptr<Symbol> createVariable(const std::string & name, bool value, const std::string & context) {
        return createVariable(name, Symbols::Value(value), context, Symbols::Variables::Type::VT_BOOLEAN);
    }
};

}  // namespace Symbols

#endif
