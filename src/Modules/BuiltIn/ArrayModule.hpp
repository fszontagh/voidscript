// ArrayModule.hpp
#ifndef MODULES_ARRAYMODULE_HPP
#define MODULES_ARRAYMODULE_HPP

#include <stdexcept>
#include <string>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

/**
 * @brief Module providing a sizeof() function for array variables.
 * Usage:
 *   sizeof($array)   -> returns number of elements in the array
 */
class ArrayModule : public BaseModule {
  public:
    void registerModule() override {
        std::vector<FunctParameterInfo> params = {
            { "array", Symbols::Variables::Type::OBJECT, "The array/object to get the size of" }
        };

        REGISTER_FUNCTION("sizeof", Symbols::Variables::Type::INTEGER, params, "Get the size of an array or object",
                          Modules::ArrayModule::SizeOf);
    }

    static Symbols::Value SizeOf(FunctionArguments & args) {
        if (args.size() != 1) {
            throw std::runtime_error("sizeof expects exactly one argument");
        }
        const auto & val  = args[0];
        auto         type = val.getType();
        switch (type) {
            case Symbols::Variables::Type::OBJECT:
                {
                    const auto & map = std::get<Symbols::Value::ObjectMap>(val.get());
                    return Symbols::Value(static_cast<int>(map.size()));
                }
            case Symbols::Variables::Type::STRING:
                {
                    const auto & str = std::get<std::string>(val.get());
                    return Symbols::Value(static_cast<int>(str.size()));
                }
            case Symbols::Variables::Type::CLASS:
                {
                    const auto & map = std::get<Symbols::Value::ObjectMap>(val.get());
                    return Symbols::Value(static_cast<int>(map.size()));
                }
            case Symbols::Variables::Type::INTEGER:
            case Symbols::Variables::Type::DOUBLE:
            case Symbols::Variables::Type::FLOAT:
            case Symbols::Variables::Type::BOOLEAN:
                {
                    return Symbols::Value(1);
                }
            default:
                throw std::runtime_error("sizeof unsupported type");
        }
    }  // SizeOf
};

}  // namespace Modules

#endif  // MODULES_ARRAYMODULE_HPP
