// PluginInit.cpp
#include "PluginInit.h"

#include <memory>

#include "CurlModule.hpp"
#include "Symbols/SymbolContainer.hpp"

/**
 * @brief Plugin initialization. Registers CurlModule functions with SymbolContainer.
 */
extern "C" void plugin_init() {
    auto module = std::make_unique<Modules::CurlModule>();
    module->setModuleName("Curl");
    
    // Register and store the module using the new pattern
    Symbols::SymbolContainer::instance()->registerModule(Modules::make_base_module_ptr(std::move(module)));
}
