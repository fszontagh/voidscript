

#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include "Symbols/SymbolContainer.hpp"

// Ensures SymbolContainer is initialized before any test or test discovery
struct SymbolContainerTestInitializer {
    SymbolContainerTestInitializer() { Symbols::SymbolContainer::initialize("testScope"); }
};

static SymbolContainerTestInitializer symbolContainerTestInitializerInstance;

#include <cassert>
#include <iostream>
#include <memory>
#include <string>

#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableSymbol.hpp"

TEST_CASE("SymbolContainer stores and restores symbols", "[symbol_container]") {
    const std::string test_scope_name = "testScope";
    const std::string variable_name   = "testVariable";
    int               variable_value  = 42;  // Example integer value

    Symbols::SymbolContainer * container = Symbols::SymbolContainer::instance();

    // Create a ValuePtr and store it in the SymbolContainer
    Symbols::ValuePtr originalValue = variable_value;

    // Use VariableSymbol, not abstract Symbol
    Symbols::SymbolPtr symbol = std::make_shared<Symbols::VariableSymbol>(
        variable_name, originalValue, container->currentScopeName(), Symbols::Variables::Type::INTEGER);

    const auto ns = container->add(symbol);

    // Retrieve the ValuePtr from the SymbolContainer
    Symbols::SymbolPtr retrievedSymbol = container->get(ns, variable_name);
    int                val             = retrievedSymbol->getValue();
    auto symbolValue = retrievedSymbol->getValue();

    REQUIRE(retrievedSymbol != nullptr);
    REQUIRE(retrievedSymbol->getKind() == Symbols::Kind::Variable);
    REQUIRE(retrievedSymbol->getValue().get<int>() == variable_value);
    REQUIRE(retrievedSymbol->getValue() == Symbols::Variables::Type::INTEGER);
    REQUIRE(retrievedSymbol->name() == variable_name);
}
