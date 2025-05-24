// DEPRECATED: This file is part of the legacy class/module management system.
// Use Symbols::ClassRegistry and related classes instead.
//
// TRANSITION: This module now delegates class management operations to Symbols::ClassRegistry
// while maintaining backward compatibility. It should ONLY be used directly from modules
// (built-in and dynamic). All other code should use Symbols::ClassRegistry.

#ifndef MODULES_UNIFIEDMODULEMANAGER_HPP
#define MODULES_UNIFIEDMODULEMANAGER_HPP

#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Symbols/ClassRegistry.hpp"

#if defined(_WIN32) || defined(_WIN64)
#    include <windows.h>
#else
#    include <dlfcn.h>
#endif

#include "BaseModule.hpp"
#include "Parser/ParsedExpression.hpp"
#include "Symbols/ClassRegistry.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

/**
 * @brief Documentation structure for function parameters.
 */
struct FunctParameterInfo {
    std::string              name;                 // the name of the parameter
    Symbols::Variables::Type type;                 // the type of the parameter
    std::string              description;          // the description of the parameter
    bool                     optional    = false;  // if the parameter is optional
    bool                     interpolate = false;  // if the parameter is interpolated
};

struct FunctionDoc {
    std::string                     name;           // function/method name
    Symbols::Variables::Type        returnType;     // return type
    std::vector<FunctParameterInfo> parameterList;  // parameter list
    std::string                     description;    // short description
};

using FunctionArguments = const std::vector<Symbols::ValuePtr>;
using CallbackFunction  = std::function<Symbols::ValuePtr(FunctionArguments &)>;

struct ClassInfo {
    struct MethodInfo {
        Symbols::Variables::Type        returnType;
        std::vector<FunctParameterInfo> parameters;
    };

    // Add methods map to ClassInfo
    std::unordered_map<std::string, Modules::ClassInfo::MethodInfo> methods;

    struct PropertyInfo {
        std::string                 name;
        Symbols::Variables::Type    type;
        Parser::ParsedExpressionPtr defaultValueExpr;
    };

    std::vector<Modules::ClassInfo::PropertyInfo>      properties;
    std::vector<std::string>                           methodNames;
    std::string                                        constructorName;   // Added
    std::unordered_map<std::string, Symbols::ValuePtr> objectProperties;  // Store object-specific properties
};

class UnifiedModuleManager {
  public:
    // Singleton instance
    static UnifiedModuleManager & instance();

    // --- Module management ---
    void                                    addModule(std::unique_ptr<BaseModule> module);  // Add a module
    void                                    registerAll();                                  // Register all modules
    void                                    loadPlugins(const std::string & directory);  // Load plugins from directory
    const std::vector<FunctParameterInfo> & getMethodParameters(const std::string & className,
                                                                const std::string & methodName) const;
    const FunctionDoc &                     getFunctionDoc(const std::string & funcName) const;

    void loadPlugin(const std::string & path);  // Load a single plugin

    // --- Function registration ---
    void                     registerFunction(const std::string & name, CallbackFunction cb,
                                              const Symbols::Variables::Type & returnType = Symbols::Variables::Type::NULL_TYPE);
    void                     registerDoc(const std::string & ModName, const FunctionDoc & doc);
    bool                     hasFunction(const std::string & name) const;
    Symbols::ValuePtr        callFunction(const std::string & name, FunctionArguments & args) const;
    Symbols::Variables::Type getFunctionReturnType(const std::string & name);
    Symbols::ValuePtr        getFunctionNullValue(const std::string & name);

    // --- Method registration (separate from functions) ---
    void              registerMethod(const std::string & className, const std::string & methodName, CallbackFunction cb,
                                     const Symbols::Variables::Type & returnType = Symbols::Variables::Type::NULL_TYPE);
    bool              hasMethod(const std::string & className, const std::string & methodName) const;
    Symbols::ValuePtr callMethod(const std::string & className, const std::string & methodName,
                                 FunctionArguments & args) const;
    Symbols::Variables::Type getMethodReturnType(const std::string & className, const std::string & methodName);
    std::vector<std::string> getMethodNames(const std::string & className) const;

    // --- Class registration ---
    bool        hasClass(const std::string & className) const;
    void        registerClass(const std::string & className, const std::string & scopeName);
    ClassInfo & getClassInfo(const std::string & className);
    void addProperty(const std::string & className, const std::string & propertyName, Symbols::Variables::Type type,
                     Parser::ParsedExpressionPtr defaultValueExpr = nullptr);
    void addMethod(const std::string & className, const std::string & methodName);
    void addMethod(const std::string & className, const std::string & methodName,
                   std::function<Symbols::ValuePtr(const std::vector<Symbols::ValuePtr> &)> cb,
                   const Symbols::Variables::Type & returnType = Symbols::Variables::Type::NULL_TYPE);
    // Added setConstructor
    void setConstructor(const std::string & className, const std::string & constructorName);
    bool hasProperty(const std::string & className, const std::string & propertyName) const;
    std::vector<std::string> getClassNames() const;

    // --- Object property management ---
    void setObjectProperty(const std::string & className, const std::string & propertyName,
                           const Symbols::ValuePtr & value);

    template <typename T>
    void setObjectProperty(const std::string & className, const std::string & propertyName, const T & value) {
        auto & classEntry                              = findOrThrow(classes_, className, "Class not found");
        classEntry.info.objectProperties[propertyName] = value;
    }

    Symbols::ValuePtr getObjectProperty(const std::string & className, const std::string & propertyName) const;
    void              deleteObjectProperty(const std::string & className, const std::string & propertyName);
    bool              hasObjectProperty(const std::string & className, const std::string & propertyName) const;
    void              clearObjectProperties(const std::string & className);

    // --- Utility functions ---
    std::vector<std::string>  getFunctionNamesForModule(BaseModule * module) const;
    std::vector<std::string>  getMethodNamesForModules(BaseModule * module) const;
    std::vector<std::string>  getMethodNamesForModuleClasses(BaseModule * module, const std::string &className) const;
    std::vector<std::string>  getPluginPaths() const;
    std::vector<BaseModule *> getPluginModules() const;
    BaseModule *              getCurrentModule() const;
    std::string               getCurrentModuleName() const;
    void                      generateMarkdownDocs(const std::string & outputDir) const;

    // Get the module for a class (for macro compatibility)
    BaseModule * getClassModule(const std::string & clsname) const;

    // Destructor
    ~UnifiedModuleManager();

  private:
    UnifiedModuleManager()                                         = default;
    UnifiedModuleManager(const UnifiedModuleManager &)             = delete;
    UnifiedModuleManager & operator=(const UnifiedModuleManager &) = delete;

    // Helper methods to reduce duplication
    template <typename T>
    const T & findOrThrow(const std::unordered_map<std::string, T> & map, const std::string & key,
                          const std::string & errorMsg) const {
        auto it = map.find(key);
        if (it == map.end()) {
            throw std::runtime_error(errorMsg + ": " + key);
        }
        return it->second;
    }

    template <typename T>
    T & findOrThrow(std::unordered_map<std::string, T> & map, const std::string & key, const std::string & errorMsg) {
        auto it = map.find(key);
        if (it == map.end()) {
            throw std::runtime_error(errorMsg + ": " + key);
        }
        return it->second;
    }

    // Helper for module name normalization
    static std::string normalizeModuleName(const std::string & name) {
        if (name.rfind("lib", 0) == 0) {
            return name.substr(3);
        }
        return name;
    }

    // Helper for creating qualified method names
    static std::string createQualifiedMethodName(const std::string & className, const std::string & methodName) {
        return className + Symbols::SymbolContainer::SCOPE_SEPARATOR + methodName;
    }

    // --- Module management ---
    struct PluginInfo {
        void *       handle;
        std::string  path;
        BaseModule * module;
    };

    std::vector<std::unique_ptr<BaseModule>> modules_;
    std::vector<PluginInfo>                  plugins_;
    BaseModule *                             currentModule_ = nullptr;

    // --- Registry entries ---
    struct RegistryEntry {
        BaseModule * module;
        FunctionDoc  doc;
    };

    struct FunctionEntry : public RegistryEntry {
        std::shared_ptr<CallbackFunction> callback;
        Symbols::Variables::Type          returnType;
    };

    struct ClassEntry : public RegistryEntry {
        ClassInfo   info;
        std::string scope;
    };

    // --- Registries ---
    std::unordered_map<std::string, FunctionEntry> functions_;
    std::unordered_map<std::string, ClassEntry>    classes_;

    // Separate storage for methods (qualified name -> method info)
    std::unordered_map<std::string, FunctionEntry> methods_;

    // Helper for documentation generation
    void writeDoc(std::ofstream & file, const std::string & name, const RegistryEntry & entry,
                  const std::string & prefix, const Symbols::Variables::Type & returnType) const;
};

}  // namespace Modules

#define REGISTER_FUNCTION(fnName, retType, paramListVec, docStr, callback)         \
    do {                                                                            \
        UnifiedModuleManager::instance().registerFunction(fnName, callback, retType); \
        UnifiedModuleManager::instance().registerDoc(                               \
            Modules::UnifiedModuleManager::instance().getCurrentModule()->name(),   \
            FunctionDoc{ fnName, retType, paramListVec, docStr });                  \
        /* Note: Functions are standalone and should NOT be registered as methods */ \
        /* with empty class names in ClassRegistry. Functions are handled by */     \
        /* UnifiedModuleManager, methods are handled by ClassRegistry separately */ \
    } while (0)

#define REGISTER_CLASS(className)                                                                            \
    do {                                                                                                     \
        /* Check if the class is already registered in ClassRegistry */                                      \
        if (!Symbols::ClassRegistry::instance().hasClass(className)) {                                       \
            /* Register in ClassRegistry */                                                                  \
            Symbols::ClassRegistry::instance().registerClass(                                                \
                className, Modules::UnifiedModuleManager::instance().getCurrentModule());                    \
        }                                                                                                    \
        /* Register in UnifiedModuleManager for backward compatibility */                                    \
        /* Note: This will update the class in UnifiedModuleManager if it already exists in ClassRegistry */ \
        UnifiedModuleManager::instance().registerClass(                                                      \
            className, Modules::UnifiedModuleManager::instance().getCurrentModule()->name());                \
    } while (0)

#define REGISTER_METHOD(className, methodName, paramList, callback, retType, docStr)                      \
    do {                                                                                                  \
        std::string fullMethodName =                                                                      \
            std::string(className) + Symbols::SymbolContainer::SCOPE_SEPARATOR + std::string(methodName); \
        UnifiedModuleManager::instance().registerMethod(className, methodName, callback, retType);        \
        UnifiedModuleManager::instance().addMethod(className, methodName, callback, retType);             \
        UnifiedModuleManager::instance().registerDoc(                                                     \
            Modules::UnifiedModuleManager::instance().getCurrentModule()->name(),                         \
            FunctionDoc{ fullMethodName, retType, paramList, docStr });                                   \
    } while (0)

#define REGISTER_PROPERTY(className, propertyName, type, defaultValue) \
    UnifiedModuleManager::instance().addProperty(className, propertyName, type, defaultValue)

#endif  // MODULES_UNIFIEDMODULEMANAGER_HPP
