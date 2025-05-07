// PluginInit.cpp
#include <memory>

#include "Modules/UnifiedModuleManager.hpp"
#include "XmlModule.hpp"

extern "C" void plugin_init() {
    // Register MariaDBModule
    Modules::UnifiedModuleManager::instance().addModule(std::make_unique<Modules::XmlModule>());
}
