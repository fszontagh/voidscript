#include <memory>
#include "CompressModule.hpp"
#include "Symbols/SymbolContainer.hpp"

extern "C" void plugin_init() {
    auto module = std::make_unique<Modules::CompressModule>();
    module->setModuleName("Compress");
    Symbols::SymbolContainer::instance()->registerModule(Modules::make_base_module_ptr(std::move(module)));
}
