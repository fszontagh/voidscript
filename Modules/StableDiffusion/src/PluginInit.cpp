// PluginInit.cpp
#include <memory>

#include "SDModule.hpp"
#include "Symbols/SymbolContainer.hpp"

/**
 * @brief Plugin initialization. Registers the StableDiffusion class with SymbolContainer.
 */
extern "C" void plugin_init() {
    auto module = std::make_unique<Modules::SDModule>();
    module->setModuleName("StableDiffusion");
    Symbols::SymbolContainer::instance()->registerModule(Modules::make_base_module_ptr(std::move(module)));
}
