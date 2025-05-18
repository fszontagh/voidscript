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
    static std::shared_ptr<Symbol> createVariable(const std::string & name, Symbols::ValuePtr value,
                                                  const std::string & context) {
        return std::make_shared<VariableSymbol>(name, value, context, value->getType());
    }

    static std::shared_ptr<Symbol> createVariable(const std::string & name, Symbols::ValuePtr value,
                                                  const std::string & context, Variables::Type type) {
        return std::make_shared<VariableSymbol>(name, value, context, type);
    }

    static std::shared_ptr<Symbol> createConstant(const std::string & name, Symbols::ValuePtr value,
                                                  const std::string & context) {
        return std::make_shared<ConstantSymbol>(name, value, context);
    }

    static std::shared_ptr<Symbol> createFunction(const std::string & name, const std::string & context,
                                                  const Symbols::FunctionParameterInfo & parameters = {}) {
        return std::make_shared<FunctionSymbol>(name, context, parameters);
    }

    static std::shared_ptr<Symbol> createFunction(const std::string & name, const std::string & context,
                                                  const Symbols::FunctionParameterInfo & parameters,
                                                  const std::string &                    plainBody) {
        return std::make_shared<FunctionSymbol>(name, context, parameters, plainBody);
    }

    static std::shared_ptr<Symbol> createFunction(const std::string & name, const std::string & context,
                                                  const Symbols::FunctionParameterInfo & parameters,
                                                  const std::string & plainBody, Symbols::Variables::Type returnType) {
        return std::make_shared<FunctionSymbol>(name, context, parameters, plainBody, returnType);
    }

    // Overloadok
    static std::shared_ptr<Symbol> createVariable(const std::string & name, int value, const std::string & context) {
        return createVariable(name, Symbols::ValuePtr::create(value), context, Symbols::Variables::Type::INTEGER);
    }

    static std::shared_ptr<Symbol> createVariable(const std::string & name, double value, const std::string & context) {
        return createVariable(name, Symbols::ValuePtr::create(value), context, Symbols::Variables::Type::DOUBLE);
    }

    static std::shared_ptr<Symbol> createVariable(const std::string & name, float value, const std::string & context) {
        return createVariable(name, Symbols::ValuePtr::create(value), context, Symbols::Variables::Type::FLOAT);
    }

    static std::shared_ptr<Symbol> createVariable(const std::string & name, const std::string & value,
                                                  const std::string & context) {
        return createVariable(name, Symbols::ValuePtr::create(value), context, Symbols::Variables::Type::STRING);
    }

    static std::shared_ptr<Symbol> createVariable(const std::string & name, bool value, const std::string & context) {
        return createVariable(name, Symbols::ValuePtr::create(value), context,
                              Symbols::Variables::Type::BOOLEAN);
    }
};

}  // namespace Symbols

#endif
