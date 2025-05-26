// PluginInit.cpp
#include <memory>

#include "MariaDBModule.hpp"
#include "Symbols/SymbolContainer.hpp"

extern "C" void plugin_init() {
    // Register MariaDBModule functions
    auto module = std::make_unique<Modules::MariaDBModule>();
    module->setModuleName("MariaDb");
    
    // Register and store the module using the new pattern
    Symbols::SymbolContainer::instance()->registerModule(Modules::make_base_module_ptr(std::move(module)));
}
