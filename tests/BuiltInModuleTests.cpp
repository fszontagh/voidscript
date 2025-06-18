#include <catch2/catch_test_macros.hpp>
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableSymbol.hpp"

using namespace Symbols;

TEST_CASE("BuiltIn modules basic functionality", "[BuiltInModules]") {
    // Initialize the symbol container for tests
    SymbolContainer::initialize("builtin_test_scope");
    auto* container = SymbolContainer::instance();
    
    SECTION("Symbol container can be initialized") {
        REQUIRE(container != nullptr);
        REQUIRE(container->currentScopeName() == "builtin_test_scope");
    }
    
    SECTION("Symbol container supports basic operations") {
        // Test basic variable operations
        auto testVar = std::make_shared<Symbols::VariableSymbol>(
            "test_var",
            ValuePtr(42),
            "builtin_test_scope",
            Variables::Type::INTEGER
        );
        
        REQUIRE_NOTHROW(container->addVariable(testVar));
        REQUIRE(container->getVariable("test_var") != nullptr);
    }
}

TEST_CASE("BuiltIn module registration", "[BuiltInModules]") {
    SymbolContainer::initialize("builtin_test_scope_2");
    auto* container = SymbolContainer::instance();
    
    SECTION("Functions can be registered and retrieved") {
        // Register a simple test function
        auto testFunc = [](const std::vector<ValuePtr>& args) -> ValuePtr {
            return ValuePtr(true);
        };
        
        container->registerFunction("test_function", testFunc, Variables::Type::BOOLEAN);
        
        REQUIRE(container->hasFunction("test_function"));
        REQUIRE(container->getFunctionReturnType("test_function") == Variables::Type::BOOLEAN);
    }
    
    SECTION("Classes can be registered") {
        auto& classInfo = container->registerClass("TestClass");
        REQUIRE(container->hasClass("TestClass"));
        REQUIRE(classInfo.name == "TestClass");
    }
}

TEST_CASE("Value creation and manipulation", "[BuiltInModules]") {
    SECTION("ValuePtr can hold different types") {
        ValuePtr intVal(42);
        ValuePtr strVal("hello");
        ValuePtr boolVal(true);
        ValuePtr doubleVal(3.14);
        
        REQUIRE(intVal.getType() == Variables::Type::INTEGER);
        REQUIRE(strVal.getType() == Variables::Type::STRING);
        REQUIRE(boolVal.getType() == Variables::Type::BOOLEAN);
        REQUIRE(doubleVal.getType() == Variables::Type::DOUBLE);
    }
    
    SECTION("ValuePtr conversions work") {
        ValuePtr intVal(42);
        ValuePtr strVal("hello");
        ValuePtr boolVal(true);
        
        REQUIRE(intVal.get<int>() == 42);
        REQUIRE(strVal.get<std::string>() == "hello");
        REQUIRE(boolVal.get<bool>() == true);
    }
    
    SECTION("ObjectMap functionality") {
        ObjectMap obj;
        obj["key1"] = ValuePtr("value1");
        obj["key2"] = ValuePtr(123);
        
        ValuePtr objVal(obj);
        REQUIRE(objVal.getType() == Variables::Type::OBJECT);
        
        auto retrieved = objVal.get<ObjectMap>();
        REQUIRE(retrieved["key1"].get<std::string>() == "value1");
        REQUIRE(retrieved["key2"].get<int>() == 123);
    }
}