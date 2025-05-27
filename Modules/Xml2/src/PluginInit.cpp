// PluginInit.cpp
#include <memory>
#include <iostream>

#include "Symbols/SymbolContainer.hpp"
#include "XmlModule.hpp"

extern "C" void plugin_init() {
    // Create and register XmlModule using the new pattern
    auto module = std::make_unique<Modules::XmlModule>();
    module->setModuleName("Xml2");
    
    // Register and store the module in one call
    Symbols::SymbolContainer::instance()->registerModule(Modules::make_base_module_ptr(std::move(module)));
}
