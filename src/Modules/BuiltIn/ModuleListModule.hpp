// ModuleListModule.hpp
#ifndef MODULES_MODULELISTMODULE_HPP
#define MODULES_MODULELISTMODULE_HPP

#include "Modules/BaseModule.hpp"

namespace Modules {

/**
 * @brief Built-in module providing a function to list loaded plugin modules.
 */
class ModuleListModule : public BaseModule {
  public:
    void registerModule() override;
};

}  // namespace Modules
#endif  // MODULES_MODULELISTMODULE_HPP