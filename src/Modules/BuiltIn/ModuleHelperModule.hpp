// ModuleHelperModule.hpp
#ifndef MODULES_MODULEHELPERMODULE_HPP
#define MODULES_MODULEHELPERMODULE_HPP

#include "Modules/BaseModule.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief Module providing helper functions for module information and documentation.
 */
class ModuleHelperModule : public BaseModule {
  public:
    ModuleHelperModule() { setModuleName("ModuleHelper"); }

    void registerModule() override;

  private:
    // Helper methods for building module information
    Symbols::Value::ObjectMap buildModuleInfoMap(BaseModule* module, const std::string& path,
                                               const UnifiedModuleManager& umm);
    
    // Helper methods for building entity information
    Symbols::Value::ObjectMap buildClassInfoMap(const std::string& className, 
                                              const UnifiedModuleManager& umm);
    Symbols::Value::ObjectMap buildFunctionInfoMap(const std::string& functionName,
                                                 const UnifiedModuleManager& umm);
    Symbols::Value::ObjectMap buildMethodInfoMap(const std::string& className,
                                               const std::string& methodName,
                                               const UnifiedModuleManager& umm);
    
    // Helper methods for building documentation
    Symbols::Value::ObjectMap buildParameterInfoMap(const FunctParameterInfo& param);
    Symbols::Value::ObjectMap buildFunctionDocMap(const std::string& functionName,
                                                const UnifiedModuleManager& umm);
};

}  // namespace Modules
#endif  // MODULES_MODULEHELPERMODULE_HPP
