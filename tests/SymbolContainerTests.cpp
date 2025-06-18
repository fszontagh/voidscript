#include <catch2/catch_test_macros.hpp>
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableSymbol.hpp"
#include "Symbols/FunctionSymbol.hpp"

using namespace Symbols;

TEST_CASE("SymbolContainer initialization", "[SymbolContainer]") {
    SECTION("Can initialize and get instance") {
        SymbolContainer::initialize("test_scope_unique");
        auto* container = SymbolContainer::instance();
        
        REQUIRE(container != nullptr);
        REQUIRE(container->currentScopeName() == "test_scope_unique");
    }
    
    SECTION("Can create and enter scopes") {
        // Work with the existing singleton state
        auto* container = SymbolContainer::instance();
        
        // Create a new scope for this test
        container->create("root_scope_test");
        REQUIRE(container->currentScopeName() == "root_scope_test");
        
        container->create("child_scope");
        REQUIRE(container->currentScopeName() == "child_scope");
        
        container->enterPreviousScope();
        REQUIRE(container->currentScopeName() == "root_scope_test");
    }
}

TEST_CASE("SymbolContainer variable operations", "[SymbolContainer]") {
    SymbolContainer::initialize("var_test_scope");
    auto* container = SymbolContainer::instance();
    
    SECTION("Can add and retrieve variables") {
        auto testVar = std::make_shared<VariableSymbol>(
            "test_var", 
            ValuePtr(42), 
            "var_test_scope",
            Variables::Type::INTEGER
        );
        
        REQUIRE_NOTHROW(container->addVariable(testVar));
        
        auto retrieved = container->getVariable("test_var");
        REQUIRE(retrieved != nullptr);
        REQUIRE(retrieved->name() == "test_var");
    }
    
    SECTION("Variables have correct scope resolution") {
        container->create("inner_scope");
        
        auto outerVar = std::make_shared<VariableSymbol>(
            "outer_var", 
            ValuePtr("outer"), 
            "var_test_scope",
            Variables::Type::STRING
        );
        container->addVariable(outerVar, "var_test_scope");
        
        auto innerVar = std::make_shared<VariableSymbol>(
            "inner_var", 
            ValuePtr("inner"), 
            "inner_scope",
            Variables::Type::STRING
        );
        container->addVariable(innerVar);
        
        // Should find both variables from inner scope
        REQUIRE(container->getVariable("inner_var") != nullptr);
        REQUIRE(container->getVariable("outer_var") != nullptr);
        
        container->enterPreviousScope();
        
        // Should find outer but not inner from outer scope
        REQUIRE(container->getVariable("outer_var") != nullptr);
        REQUIRE(container->getVariable("inner_var") == nullptr);
    }
}

TEST_CASE("SymbolContainer function operations", "[SymbolContainer]") {
    SymbolContainer::initialize("func_test_scope");
    auto* container = SymbolContainer::instance();
    
    SECTION("Can register and call functions") {
        auto testFunc = [](const std::vector<ValuePtr>& args) -> ValuePtr {
            return ValuePtr(42);
        };
        
        container->registerFunction("test_func", testFunc, Variables::Type::INTEGER);
        
        REQUIRE(container->hasFunction("test_func"));
        REQUIRE(container->getFunctionReturnType("test_func") == Variables::Type::INTEGER);
        
        std::vector<ValuePtr> args;
        auto result = container->callFunction("test_func", args);
        REQUIRE(result.getType() == Variables::Type::INTEGER);
        REQUIRE(result.get<int>() == 42);
    }
}

TEST_CASE("SymbolContainer class operations", "[SymbolContainer]") {
    SymbolContainer::initialize("class_test_scope");
    auto* container = SymbolContainer::instance();
    
    SECTION("Can register and query classes") {
        auto& classInfo = container->registerClass("TestClass");
        
        REQUIRE(container->hasClass("TestClass"));
        REQUIRE(classInfo.name == "TestClass");
        
        // Add a property
        container->addProperty("TestClass", "test_prop", Variables::Type::STRING);
        REQUIRE(container->hasProperty("TestClass", "test_prop"));
        
        // Add a method
        container->addMethod("TestClass", "test_method", Variables::Type::INTEGER);
        REQUIRE(container->hasMethod("TestClass", "test_method"));
    }
}