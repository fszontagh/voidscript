// PluginInit.cpp
#include <memory>

#include "ImagickModule.hpp"
#include "Modules/UnifiedModuleManager.hpp"

extern "C" void plugin_init() {
    // Register MariaDBModule
    Modules::UnifiedModuleManager::instance().addModule(std::make_unique<Modules::ImagickModule>());
}
