// BaseModule.hpp
#ifndef MODULES_BASEMODULE_HPP
#define MODULES_BASEMODULE_HPP

#include <string>
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"

namespace Modules {

/**
 * @brief Base class for modules that can register functions and variables into the symbol table.
 */
class BaseModule {
  public:
    BaseModule() = default;
    virtual ~BaseModule() = default;

    /**
     * @brief Register this module's symbols (functions, variables) into the global symbol container.
     * Modules should use Symbols::SymbolContainer::instance() and SymbolFactory to add symbols.
     */
    virtual void registerModule() = 0;
};

} // namespace Modules
#endif // MODULES_BASEMODULE_HPP