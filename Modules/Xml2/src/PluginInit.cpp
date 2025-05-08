// PluginInit.cpp
#include <memory>
#include <iostream>

#include "Modules/UnifiedModuleManager.hpp"
#include "XmlModule.hpp"

extern "C" void plugin_init() {
    // Register XmlModule
    auto module = std::make_unique<Modules::XmlModule>();
    module->setModuleName("Xml2");
    Modules::UnifiedModuleManager::instance().addModule(std::move(module));
}
