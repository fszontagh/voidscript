// PluginInit.cpp
#include <memory>

#include "HashModule.hpp"
#include "Symbols/SymbolContainer.hpp"

/**
 * @brief Plugin initialization. Registers HashModule functions with SymbolContainer.
 */
extern "C" void plugin_init() {
    auto module = std::make_unique<Modules::HashModule>();
    module->setModuleName("Hash");
    Symbols::SymbolContainer::instance()->registerModule(Modules::make_base_module_ptr(std::move(module)));
}
