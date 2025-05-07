// PluginInit.cpp
#include <memory>

#include "FormatModule.hpp"
#include "Modules/UnifiedModuleManager.hpp"

/**
 * @brief Plugin initialization. Registers FormatModule with the ModuleManager.
 */
extern "C" void plugin_init() {
    Modules::UnifiedModuleManager::instance().addModule(std::make_unique<Modules::FormatModule>());
}
