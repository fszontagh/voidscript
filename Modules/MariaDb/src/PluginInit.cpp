// PluginInit.cpp
#include <memory>

#include "MariaDBModule.hpp"
#include "Symbols/SymbolContainer.hpp"

extern "C" void plugin_init() {
    // Register MariaDBModule functions
    auto module = std::make_unique<Modules::MariaDBModule>();
    
    // Set as current module for registration macros
    Symbols::SymbolContainer::instance()->setCurrentModule(module.get());
    module->registerFunctions();
    Symbols::SymbolContainer::instance()->setCurrentModule(nullptr);
}
