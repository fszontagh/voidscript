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
        mgr.registerFunction("sizeof", [](const std::vector<Symbols::Value> & args) {
            using namespace Symbols;
            if (args.size() != 1) {
                throw std::runtime_error("sizeof expects exactly one argument");
            }
            const auto & val  = args[0];
            auto   type = val.getType();
            // Only allow array types (OBJECT)
            if (type == Variables::Type::OBJECT) {
                const auto & map = std::get<Value::ObjectMap>(val.get());
                return Value(static_cast<int>(map.size()));
            }
            throw std::runtime_error("sizeof expects an array variable");
        });
    }
};

}  // namespace Modules

#endif  // MODULES_ARRAYMODULE_HPP
