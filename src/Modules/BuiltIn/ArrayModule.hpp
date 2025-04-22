// ArrayModule.hpp
#ifndef MODULES_ARRAYMODULE_HPP
#define MODULES_ARRAYMODULE_HPP

#include <stdexcept>
#include <string>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Modules/ModuleManager.hpp"
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
        auto & mgr = ModuleManager::instance();
        mgr.registerFunction("sizeof", [](FuncionArguments & args) {
            using namespace Symbols;
            if (args.size() != 1) {
                throw std::runtime_error("sizeof expects exactly one argument");
            }
            const auto & val = args[0];
            auto type = val.getType();
            switch (type) {
                case Variables::Type::OBJECT: {
                    const auto & map = std::get<Value::ObjectMap>(val.get());
                    return Value(static_cast<int>(map.size()));
                }
                case Variables::Type::STRING: {
                    const auto & str = std::get<std::string>(val.get());
                    return Value(static_cast<int>(str.size()));
                }
                case Variables::Type::CLASS: {
                    const auto & map = std::get<Value::ObjectMap>(val.get());
                    return Value(static_cast<int>(map.size()));
                }
                case Variables::Type::INTEGER:
                case Variables::Type::DOUBLE:
                case Variables::Type::FLOAT:
                case Variables::Type::BOOLEAN: {
                    return Value(1);
                }
                default:
                    throw std::runtime_error("sizeof unsupported type");
            }
        });
    }
};

}  // namespace Modules

#endif  // MODULES_ARRAYMODULE_HPP
