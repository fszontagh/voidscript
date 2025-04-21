// ModuleHelperModule.hpp
#ifndef MODULES_MODULEHELPERMODULE_HPP
#define MODULES_MODULEHELPERMODULE_HPP

#include "Modules/BaseModule.hpp"

namespace Modules {

/**
 * @brief Module providing helper functions for dynamic modules: list, exists, info.
 */
class ModuleHelperModule : public BaseModule {
  public:
    void registerModule() override;
};

}  // namespace Modules
#endif  // MODULES_MODULEHELPERMODULE_HPP