// TypeofModule.hpp
#ifndef MODULES_TYPEOFMODULE_HPP
#define MODULES_TYPEOFMODULE_HPP

#include <string>
#include <vector>
#include "BaseModule.hpp"
#include "ModuleManager.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

/**
 * @brief Module providing a typeof() function.
 * Usage:
 *   typeof($var)            -> returns string name of type ("int", "string", etc.)
 *   typeof($var, "int")   -> returns bool indicating if type matches
 */
class TypeofModule : public BaseModule {
  public:
    void registerModule() override {
        auto &mgr = ModuleManager::instance();
        mgr.registerFunction("typeof", [](const std::vector<Symbols::Value> &args) {
            using namespace Symbols;
            if (args.size() == 1) {
                auto t = args[0].getType();
                return Value(Variables::TypeToString(t));
            } else if (args.size() == 2) {
                auto t = args[0].getType();
                std::string name = Variables::TypeToString(t);
                if (args[1].getType() != Variables::Type::STRING) {
                    throw std::runtime_error("Second argument to typeof must be string");
                }
                bool match = (name == args[1].get<std::string>());
                return Value(match);
            }
            throw std::runtime_error("typeof expects 1 or 2 arguments");
        });
    }
};

} // namespace Modules

#endif // MODULES_TYPEOFMODULE_HPP