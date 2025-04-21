// VariableHelpersModule.hpp
#ifndef MODULES_VARIABLEHELPERSMODULE_HPP
#define MODULES_VARIABLEHELPERSMODULE_HPP

#include <stdexcept>
#include <string>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Modules/ModuleManager.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

/**
 * @brief Module providing helper functions for variables.
 * Currently supports:
 *   typeof($var)            -> returns string name of type
 *   typeof($var, "int")   -> returns bool if type matches
 */
class VariableHelpersModule : public BaseModule {
  public:
    void registerModule() override {
        auto & mgr = ModuleManager::instance();
        mgr.registerFunction("typeof", [](const std::vector<Symbols::Value> & args) {
            using namespace Symbols;
            if (args.size() == 1) {
                auto t = args[0].getType();
                return Value(Variables::TypeToString(t));
            }
            if (args.size() == 2) {
                auto t = args[0].getType();
                if (args[1].getType() != Variables::Type::STRING) {
                    throw std::runtime_error("Second argument to typeof must be string");
                }
                // Compare against provided type name via mapping
                const std::string provided = args[1].get<std::string>();
                auto expected = Variables::StringToType(provided);
                bool match = (t == expected);
                return Value(match);
            }
            throw std::runtime_error("typeof expects 1 or 2 arguments");
        });
    }
};

}  // namespace Modules

#endif  // MODULES_VARIABLEHELPERSMODULE_HPP
