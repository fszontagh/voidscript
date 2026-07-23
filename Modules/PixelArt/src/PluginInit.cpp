// PluginInit.cpp
#include <memory>

#include "PixelArtModule.hpp"
#include "Symbols/SymbolContainer.hpp"

/**
 * @brief Plugin initialization. Registers the PixelArt class with SymbolContainer.
 */
extern "C" void plugin_init() {
    auto module = std::make_unique<Modules::PixelArtModule>();
    module->setModuleName("PixelArt");
    Symbols::SymbolContainer::instance()->registerModule(Modules::make_base_module_ptr(std::move(module)));
}
