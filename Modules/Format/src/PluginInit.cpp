// PluginInit.cpp
#include <memory>

#include "FormatModule.hpp"
#include "Symbols/SymbolContainer.hpp"

/**
 * @brief Plugin initialization. Registers FormatModule functions with SymbolContainer.
 */
extern "C" void plugin_init() {
    auto module = std::make_unique<Modules::FormatModule>();
    module->setModuleName("Format");
    
    // Register and store the module using the new pattern
    Symbols::SymbolContainer::instance()->registerModule(Modules::make_base_module_ptr(std::move(module)));
}
