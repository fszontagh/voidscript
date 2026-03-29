// PluginInit.cpp
#include <memory>

#include "MariaDBModule.hpp"
#include "Symbols/SymbolContainer.hpp"

/**
 * @brief Plugin initialization. Registers MariaDBModule functions with SymbolContainer.
 */
extern "C" void plugin_init() {
    auto module = std::unique_ptr<Modules::MariaDBModule>(new Modules::MariaDBModule());

    // Check for duplicate registration to avoid issues
    if (Symbols::SymbolContainer::instance()->hasModule(module->name())) {
        // Module already registered, skip to avoid duplicate registrations
        return;
    }

    // Additional check: if MariaDBConnection class already has methods, skip registration
    if (Symbols::SymbolContainer::instance()->hasClass("MariaDBConnection") &&
        Symbols::SymbolContainer::instance()->hasMethod("MariaDBConnection", "connect")) {
        // Methods already registered, skip to avoid duplicate registrations
        return;
    }

    // Register and store the module using the new pattern
    Symbols::SymbolContainer::instance()->registerModule(Modules::make_base_module_ptr(std::move(module)));
}