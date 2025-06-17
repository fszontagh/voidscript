// ArrayModule.hpp
#ifndef MODULES_ARRAYMODULE_HPP
#define MODULES_ARRAYMODULE_HPP

#include <stdexcept>
#include <string>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"
#include "Symbols/RegistrationMacros.hpp"

namespace Modules {

/**
 * @brief Module providing a sizeof() function for array variables.
 * Usage:
 *   sizeof($array)   -> returns number of elements in the array
 */
class ArrayModule : public BaseModule {
  public:
    ArrayModule() {
        setModuleName("Array");
        setDescription("Provides array and object manipulation functions, including size operations for collections");
    }

    void registerFunctions() override {
        std::vector<Symbols::FunctionParameterInfo> params = {
            { "array", Symbols::Variables::Type::OBJECT, "The array/object to get the size of", false, false }
        };

        REGISTER_FUNCTION("sizeof", Symbols::Variables::Type::INTEGER, params, "Get the size of an array or object",
                          Modules::ArrayModule::SizeOf);
    }

    static Symbols::ValuePtr SizeOf(Symbols::FunctionArguments & args) {
        if (args.size() != 1) {
            throw std::runtime_error("sizeof expects exactly one argument");
        }
        const auto & val  = args[0];
        auto         type = val->getType();
        switch (type) {
            case Symbols::Variables::Type::OBJECT:
                {
                    const auto & map = val->get<Symbols::ObjectMap>();
                    return static_cast<int>(map.size());
                }
            case Symbols::Variables::Type::STRING:
                {
                    const auto & str = val->get<std::string>();
                    return static_cast<int>(str.size());
                }
            case Symbols::Variables::Type::CLASS:
                {
                    const auto & map = val->get<Symbols::ObjectMap>();
                    return static_cast<int>(map.size());
                }
            case Symbols::Variables::Type::INTEGER:
            case Symbols::Variables::Type::DOUBLE:
            case Symbols::Variables::Type::FLOAT:
            case Symbols::Variables::Type::BOOLEAN:
                {
                    return 1;
                }
            default:
                throw std::runtime_error("sizeof unsupported type: "+Symbols::Variables::TypeToString(val));
        }
    }  // SizeOf
};

}  // namespace Modules

#endif  // MODULES_ARRAYMODULE_HPP
