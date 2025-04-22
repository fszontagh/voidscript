// PluginInit.cpp
#include <memory>

#include "XmlModule.hpp"
#include "Modules/ModuleManager.hpp"

extern "C" void plugin_init() {
    // Register MariaDBModule
    Modules::ModuleManager::instance().addModule(std::make_unique<Modules::XmlModule>());
}
