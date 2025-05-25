// PluginInit.cpp
#include <memory>

#include "ImagickModule.hpp"
#include "Symbols/SymbolContainer.hpp"

extern "C" void plugin_init() {
    auto module = std::make_unique<Modules::ImagickModule>();
    module->registerFunctions();
}
