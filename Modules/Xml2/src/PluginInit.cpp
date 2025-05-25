// PluginInit.cpp
#include <memory>
#include <iostream>

#include "Symbols/SymbolContainer.hpp"
#include "XmlModule.hpp"

extern "C" void plugin_init() {
    // Register XmlModule
    auto module = std::make_unique<Modules::XmlModule>();
    module->setModuleName("Xml2");
    
    // Set as current module for registration macros
    Symbols::SymbolContainer::instance()->setCurrentModule(module.get());
    module->registerFunctions();
    Symbols::SymbolContainer::instance()->setCurrentModule(nullptr);
}
