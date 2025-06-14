// PluginInit.cpp
#include <memory>

#include "ImagickModule.hpp"
#include "Symbols/SymbolContainer.hpp"

extern "C" void plugin_init() {
    auto module = std::make_unique<Modules::ImagickModule>();
    module->setModuleName("Imagick");

    // Register and store the module using the new pattern
    Symbols::SymbolContainer::instance()->registerModule(Modules::make_base_module_ptr(std::move(module)));
}
