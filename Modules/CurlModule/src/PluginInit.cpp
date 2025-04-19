// PluginInit.cpp
#include "PluginInit.h"
#include <memory>
#include "Modules/ModuleManager.hpp"
#include "CurlModule.hpp"

/**
 * @brief Plugin initialization. Registers CurlModule with the ModuleManager.
 */
extern "C" void plugin_init() {
    Modules::ModuleManager::instance().addModule(
        std::make_unique<Modules::CurlModule>()
    );
}