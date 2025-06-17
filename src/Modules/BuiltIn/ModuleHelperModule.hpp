// ModuleHelperModule.hpp
#ifndef MODULES_MODULEHELPERMODULE_HPP
#define MODULES_MODULEHELPERMODULE_HPP

#include "../BaseModule.hpp"
#include "../../Symbols/FunctionParameterInfo.hpp"
#include "../../Symbols/SymbolContainer.hpp"
#include "../../Symbols/Value.hpp"
#include "../../utils.h"

namespace Modules {

/**
 * @brief Module providing helper functions for module information and documentation.
 */
class ModuleHelperModule : public BaseModule {
  public:
    ModuleHelperModule() { setModuleName("ModuleHelper"); }

    void registerFunctions() override;

  private:
    // ===== INTERNAL HELPER FUNCTIONS =====
    
    // Data gathering functions (lightweight, direct API calls)
    static std::vector<std::string> gatherModuleNames(const Symbols::SymbolContainer * sc);
    static std::vector<std::string> gatherFunctionNames(const std::string & moduleName, const Symbols::SymbolContainer * sc);
    static std::vector<std::string> gatherClassNames(const std::string & moduleName, const Symbols::SymbolContainer * sc);
    static std::vector<std::string> gatherMethodNames(const std::string & className, const Symbols::SymbolContainer * sc);
    
    // Documentation building functions (only called when detailed info is needed)
    static Symbols::ObjectMap buildParameterInfoMap(const Symbols::FunctionParameterInfo & param);
    static Symbols::ObjectMap buildFunctionDocumentation(const std::string & functionName, const Symbols::SymbolContainer * sc);
    static Symbols::ObjectMap buildMethodDocumentation(const std::string & className, const std::string & methodName, const Symbols::SymbolContainer * sc);
    
    // Utility functions for object array creation
    static Symbols::ValuePtr createStringArray(const std::vector<std::string> & items);
    static Symbols::ValuePtr createParameterArray(const std::vector<Symbols::FunctionParameterInfo> & parameters);

    // ===== PUBLIC API FUNCTIONS =====
    
    // 1. List Functions (return simple arrays)
    static Symbols::ValuePtr ListModules(const FunctionArguments & args);
    static Symbols::ValuePtr ListModuleFunctions(const FunctionArguments & args);
    static Symbols::ValuePtr ListModuleClasses(const FunctionArguments & args);
    static Symbols::ValuePtr ListClassMethods(const FunctionArguments & args);
    
    // 2. Basic Info Functions (return flat objects with basic information)
    static Symbols::ValuePtr GetFunctionInfo(const FunctionArguments & args);
    static Symbols::ValuePtr GetClassInfo(const FunctionArguments & args);
    static Symbols::ValuePtr GetMethodInfo(const FunctionArguments & args);
    static Symbols::ValuePtr GetModuleSummary(const FunctionArguments & args);
    
    // 3. Detailed Info Functions (return comprehensive objects with full documentation)
    static Symbols::ValuePtr GetFunctionDetails(const FunctionArguments & args);
    static Symbols::ValuePtr GetClassDetails(const FunctionArguments & args);
    static Symbols::ValuePtr GetMethodDetails(const FunctionArguments & args);
    static Symbols::ValuePtr GetModuleDetails(const FunctionArguments & args);
    
    // 4. Utility Functions (return boolean existence checks)
    static Symbols::ValuePtr ModuleExists(const FunctionArguments & args);
    static Symbols::ValuePtr FunctionExists(const FunctionArguments & args);
    static Symbols::ValuePtr ClassExists(const FunctionArguments & args);
    static Symbols::ValuePtr MethodExists(const FunctionArguments & args);
};

}  // namespace Modules
#endif  // MODULES_MODULEHELPERMODULE_HPP
