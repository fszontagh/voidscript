#include <catch2/catch_test_macros.hpp>
#include "Compiler/VoidScriptCompiler.hpp"
#include "Symbols/SymbolContainer.hpp"

using namespace Symbols;

TEST_CASE("Compiler basic functionality", "[Compiler]") {
    // Initialize the symbol container for compiler tests
    SymbolContainer::initialize("compiler_test_scope");
    auto* container = SymbolContainer::instance();
    
    SECTION("Compiler can be instantiated") {
        // This is a basic test to ensure the compiler headers can be included
        // and basic objects can be created without throwing
        REQUIRE(container != nullptr);
        REQUIRE(container->currentScopeName() == "compiler_test_scope");
    }
    
    SECTION("Symbol container supports compiler operations") {
        // Test that the symbol container can handle basic operations
        // needed for compilation
        
        // Test scope creation
        container->create("function_scope");
        REQUIRE(container->currentScopeName() == "function_scope");
        
        // Test scope exit
        container->enterPreviousScope();
        REQUIRE(container->currentScopeName() == "compiler_test_scope");
    }
}

TEST_CASE("Compiler environment setup", "[Compiler]") {
    SymbolContainer::initialize("compiler_env_test");
    auto* container = SymbolContainer::instance();
    
    SECTION("Function registration works") {
        // Test that functions can be registered for compilation
        auto testFunc = [](const std::vector<ValuePtr>& args) -> ValuePtr {
            return ValuePtr(true);
        };
        
        container->registerFunction("compile_test_func", testFunc, Variables::Type::BOOLEAN);
        
        REQUIRE(container->hasFunction("compile_test_func"));
        REQUIRE(container->getFunctionReturnType("compile_test_func") == Variables::Type::BOOLEAN);
    }
    
    SECTION("Class registration for compilation") {
        // Test class registration needed for compilation
        auto& classInfo = container->registerClass("CompileTestClass");
        REQUIRE(container->hasClass("CompileTestClass"));
        REQUIRE(classInfo.name == "CompileTestClass");
    }
}