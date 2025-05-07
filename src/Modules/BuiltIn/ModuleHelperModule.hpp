// ModuleHelperModule.hpp
#ifndef MODULES_MODULEHELPERMODULE_HPP
#define MODULES_MODULEHELPERMODULE_HPP

#include "Modules/BaseModule.hpp"
#include "Modules/UnifiedModuleManager.hpp"

namespace Modules {

/**
 * @brief Module providing helper functions for dynamic modules: list, exists, info.
 */
class ModuleHelperModule : public BaseModule {
  public:
    void                      registerModule() override;
    Symbols::Value::ObjectMap buildModuleInfoMap(BaseModule * module, const std::string & path,
                                                 const UnifiedModuleManager & umm);
};

}  // namespace Modules
#endif  // MODULES_MODULEHELPERMODULE_HPP
