// PluginInit.cpp
#include <memory>

#include "ArchiveModule.hpp"
#include "Symbols/SymbolContainer.hpp"

extern "C" void plugin_init() {
    auto module = std::make_unique<Modules::ArchiveModule>();
    module->setModuleName("Archive");
    Symbols::SymbolContainer::instance()->registerModule(Modules::make_base_module_ptr(std::move(module)));
}
