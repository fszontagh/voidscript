// PluginInit.cpp
#include <memory>

#include "FormatModule.hpp"
#include "Symbols/SymbolContainer.hpp"

/**
 * @brief Plugin initialization. Registers FormatModule functions with SymbolContainer.
 */
extern "C" void plugin_init() {
    auto module = std::make_unique<Modules::FormatModule>();
    module->registerFunctions();
}
