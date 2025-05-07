// PluginInit.cpp
#include "PluginInit.h"

#include <memory>

#include "CurlModule.hpp"
#include "Modules/UnifiedModuleManager.hpp"

/**
 * @brief Plugin initialization. Registers CurlModule with the ModuleManager.
 */
extern "C" void plugin_init() {
    Modules::UnifiedModuleManager::instance().addModule(std::make_unique<Modules::CurlModule>());
}
