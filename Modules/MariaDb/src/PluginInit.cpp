// PluginInit.cpp
#include <memory>

#include "MariaDBModule.hpp"
#include "Modules/UnifiedModuleManager.hpp"

extern "C" void plugin_init() {
    // Register MariaDBModule
    Modules::UnifiedModuleManager::instance().addModule(std::make_unique<Modules::MariaDBModule>());
}
