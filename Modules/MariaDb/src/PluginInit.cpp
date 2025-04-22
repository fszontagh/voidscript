// PluginInit.cpp
#include <memory>

#include "MariaDBModule.hpp"
#include "Modules/ModuleManager.hpp"

extern "C" void plugin_init() {
    // Register MariaDBModule
    Modules::ModuleManager::instance().addModule(std::make_unique<Modules::MariaDBModule>());
}
