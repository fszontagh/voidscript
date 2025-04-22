// PluginInit.cpp
#include "PluginInit.h"
#include <memory>
#include "Modules/ModuleManager.hpp"
#include "FormatModule.hpp"

/**
 * @brief Plugin initialization. Registers FormatModule with the ModuleManager.
 */
extern "C" void plugin_init() {
    Modules::ModuleManager::instance().addModule(
        std::make_unique<Modules::FormatModule>()
    );
}