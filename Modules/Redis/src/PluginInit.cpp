#include <memory>
#include "RedisModule.hpp"
#include "Symbols/SymbolContainer.hpp"

extern "C" void plugin_init() {
    auto module = std::make_unique<Modules::RedisModule>();
    module->setModuleName("Redis");
    Symbols::SymbolContainer::instance()->registerModule(Modules::make_base_module_ptr(std::move(module)));
}
