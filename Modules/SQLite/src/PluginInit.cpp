// PluginInit.cpp
#include <memory>

#include "SQLiteModule.hpp"
#include "Symbols/SymbolContainer.hpp"

/**
 * @brief Plugin initialization. Registers the SQLite class with SymbolContainer.
 */
extern "C" void plugin_init() {
    auto module = std::make_unique<Modules::SQLiteModule>();
    module->setModuleName("SQLite");
    Symbols::SymbolContainer::instance()->registerModule(Modules::make_base_module_ptr(std::move(module)));
}
