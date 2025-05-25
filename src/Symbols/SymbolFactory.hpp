// SymbolFactory.hpp
#ifndef SYMBOL_FACTORY_HPP
#define SYMBOL_FACTORY_HPP

#include <memory>
#include <string>

#include "ClassSymbol.hpp"
#include "ConstantSymbol.hpp"
#include "FunctionSymbol.hpp"
#include "MethodSymbol.hpp"
#include "VariableSymbol.hpp"

namespace Symbols {

class SymbolFactory {
  public:
    static std::shared_ptr<Symbol> createVariable(
        const std::string & name, const Symbols::ValuePtr & value, const std::string & context,
        Symbols::Variables::Type type = Symbols::Variables::Type::UNDEFINED_TYPE) {
        return std::make_shared<VariableSymbol>(
            name, value, context,
            type == Symbols::Variables::Type::UNDEFINED_TYPE ? value->getType() : type);
    }

    static std::shared_ptr<Symbol> createConstant(const std::string & name, const Symbols::ValuePtr & value,
                                                  const std::string & context) {
        return std::make_shared<ConstantSymbol>(name, value, context);
    }

    static std::shared_ptr<Symbol> createFunction(const std::string & name, const std::string & context,
                                                  const std::vector<Symbols::FunctionParameterInfo> & parameters = {}) {
        return std::make_shared<FunctionSymbol>(name, context, parameters);
    }

    static std::shared_ptr<Symbol> createFunction(const std::string & name, const std::string & context,
                                                  const std::vector<Symbols::FunctionParameterInfo> & parameters,
                                                  const std::string &                    plainBody) {
        return std::make_shared<FunctionSymbol>(name, context, parameters, plainBody);
    }

    static std::shared_ptr<Symbol> createFunction(const std::string & name, const std::string & context,
                                                  const std::vector<Symbols::FunctionParameterInfo> & parameters,
                                                  const std::string & plainBody, Symbols::Variables::Type returnType) {
        return std::make_shared<FunctionSymbol>(name, context, parameters, plainBody, returnType);
    }

    static std::shared_ptr<Symbol> createMethod(const std::string & name, 
                                                 const std::string & context,
                                                 const std::string & className,
                                                 const std::vector<Symbols::FunctionParameterInfo> & parameters = {}) {
        return std::make_shared<MethodSymbol>(name, context, className, parameters);
    }

    static std::shared_ptr<Symbol> createMethod(const std::string & name, 
                                                 const std::string & context,
                                                 const std::string & className,
                                                 const std::vector<Symbols::FunctionParameterInfo> & parameters,
                                                 const std::string & plainBody) {
        return std::make_shared<MethodSymbol>(name, context, className, parameters, plainBody);
    }

    static std::shared_ptr<Symbol> createMethod(const std::string & name, 
                                                 const std::string & context,
                                                 const std::string & className,
                                                 const std::vector<Symbols::FunctionParameterInfo> & parameters,
                                                 const std::string & plainBody, 
                                                 Symbols::Variables::Type returnType) {
        return std::make_shared<MethodSymbol>(name, context, className, parameters, plainBody, returnType);
    }

    static std::shared_ptr<Symbol> createClass(const std::string & name, 
                                                const std::string & context,
                                                const std::string & parentClass = "",
                                                bool isAbstract = false) {
        return std::make_shared<ClassSymbol>(name, context, parentClass, isAbstract);
    }

    // Overloadok
    static std::shared_ptr<Symbol> createVariable(const std::string & name, int value, const std::string & context) {
        Symbols::ValuePtr _v(value);
        return createVariable(name, _v, context, Symbols::Variables::Type::INTEGER);
    }

    static std::shared_ptr<Symbol> createVariable(const std::string & name, double value, const std::string & context) {
        Symbols::ValuePtr _v(value);
        return createVariable(name, _v, context, Symbols::Variables::Type::DOUBLE);
    }

    static std::shared_ptr<Symbol> createVariable(const std::string & name, float value, const std::string & context) {
        Symbols::ValuePtr _v(value);
        return createVariable(name, _v, context, Symbols::Variables::Type::FLOAT);
    }

    static std::shared_ptr<Symbol> createVariable(const std::string & name, const std::string & value,
                                                  const std::string & context) {
        Symbols::ValuePtr _v(value);
        return createVariable(name, _v, context, Symbols::Variables::Type::STRING);
    }

    static std::shared_ptr<Symbol> createVariable(const std::string & name, bool value, const std::string & context) {
        Symbols::ValuePtr _v(value);
        return createVariable(name, _v, context, Symbols::Variables::Type::BOOLEAN);
    }
};

}  // namespace Symbols

#endif
