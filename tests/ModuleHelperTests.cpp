#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include "Modules/BuiltIn/ModuleHelperModule.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"

using namespace Modules;
using namespace Symbols;

TEST_CASE("ModuleHelperModule functions without plugins", "[module_helper]") {
    auto & mm = UnifiedModuleManager::instance();
    // Register only the ModuleHelperModule
    mm.addModule(std::make_unique<ModuleHelperModule>());
    mm.registerAll();

    SECTION("module_list returns empty object map") {
        auto val = mm.callFunction("module_list", {});
        REQUIRE(val.getType() == Variables::Type::OBJECT);
        auto obj = val.get<Symbols::ObjectMap>();
        REQUIRE(obj.empty());
    }

    SECTION("module_exists returns false for unknown module") {
        auto val = mm.callFunction("module_exists", std::vector<ValuePtr>{ ValuePtr("nonexistent") });
        REQUIRE(val.getType() == Variables::Type::BOOLEAN);
        REQUIRE(val.get<bool>() == false);
    }

    SECTION("module_info returns null string for unknown module") {
        auto val = mm.callFunction("module_info", std::vector<ValuePtr>{ ValuePtr("nonexistent") });
        REQUIRE(val.getType() == Variables::Type::OBJECT);
        auto obj = val.get<Symbols::ObjectMap>();
        REQUIRE(obj.empty());
    }

    SECTION("invalid arguments throw exception") {
        REQUIRE_THROWS_AS(mm.callFunction("module_list", std::vector<ValuePtr>{ ValuePtr("arg") }), std::runtime_error);
        REQUIRE_THROWS_AS(mm.callFunction("module_exists", {}), std::runtime_error);
        REQUIRE_THROWS_AS(mm.callFunction("module_info", {}), std::runtime_error);
    }
}
