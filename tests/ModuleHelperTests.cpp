#include <catch2/catch_test_macros.hpp>
#include "Modules/BuiltIn/ModuleHelperModule.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

using namespace Modules;
using namespace Symbols;

TEST_CASE("ModuleHelperModule basic functionality", "[ModuleHelper]") {
    // Initialize the symbol container for tests
    SymbolContainer::initialize("test_scope");
    auto* container = SymbolContainer::instance();
    
    SECTION("ModuleHelperModule can be created") {
        ModuleHelperModule module;
        REQUIRE(module.name() == "ModuleHelper");
        REQUIRE_FALSE(module.description().empty());
    }
    
    SECTION("ModuleHelperModule registers functions") {
        ModuleHelperModule module;
        container->setCurrentModule(&module);
        
        // This should not throw
        REQUIRE_NOTHROW(module.registerFunctions());
        
        // Verify some basic functions are registered
        REQUIRE(container->hasFunction("list_modules"));
        REQUIRE(container->hasFunction("module_exists"));
    }
}

TEST_CASE("ModuleHelper function registration", "[ModuleHelper]") {
    SymbolContainer::initialize("test_scope_2");
    auto* container = SymbolContainer::instance();
    
    ModuleHelperModule module;
    container->setCurrentModule(&module);
    module.registerFunctions();
    
    SECTION("Essential functions are registered") {
        REQUIRE(container->hasFunction("list_modules"));
        REQUIRE(container->hasFunction("list_module_functions"));
        REQUIRE(container->hasFunction("list_module_classes"));
        REQUIRE(container->hasFunction("module_exists"));
        REQUIRE(container->hasFunction("function_exists"));
        REQUIRE(container->hasFunction("class_exists"));
    }
    
    SECTION("Functions can be called without error") {
        std::vector<ValuePtr> emptyArgs;
        
        // Test list_modules - should return an array
        REQUIRE_NOTHROW(container->callFunction("list_modules", emptyArgs));
        
        // Test module_exists with argument
        std::vector<ValuePtr> moduleArgs = {ValuePtr("ModuleHelper")};
        REQUIRE_NOTHROW(container->callFunction("module_exists", moduleArgs));
    }
}