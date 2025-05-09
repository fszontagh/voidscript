#ifndef MODULES_UNIFIEDMODULEMANAGER_HPP
#define MODULES_UNIFIEDMODULEMANAGER_HPP

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>

#ifndef _WIN32
#    include <dlfcn.h>
#else
#    include <windows.h>
#endif

#include "BaseModule.hpp"
#include "Parser/ParsedExpression.hpp"
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
    std::string                     name;         // function/method name
    Symbols::Variables::Type        returnType;   // return type
    std::vector<FunctParameterInfo> parameterList;// parameter list
    std::string                     description;  // short description
};

using FunctionArguments = const std::vector<Symbols::Value>;
using CallbackFunction  = std::function<Symbols::Value(FunctionArguments &)>;

struct ClassInfo {
    struct PropertyInfo {
        std::string                 name;
        Symbols::Variables::Type    type;
        Parser::ParsedExpressionPtr defaultValueExpr;
    };
    std::vector<PropertyInfo> properties;
    std::vector<std::string>  methodNames;
    std::unordered_map<std::string, Symbols::Value> objectProperties;  // Store object-specific properties
};

class UnifiedModuleManager {
  public:
    // Singleton instance
    static UnifiedModuleManager & instance();

    // --- Module management ---
    void addModule(std::unique_ptr<BaseModule> module); // Add a module
    void registerAll();                                 // Register all modules
    void loadPlugins(const std::string & directory);    // Load plugins from directory
    void loadPlugin(const std::string & path);          // Load a single plugin

    // --- Function registration ---
    void registerFunction(const std::string & name, CallbackFunction cb,
                         const Symbols::Variables::Type & returnType = Symbols::Variables::Type::NULL_TYPE);
    void registerDoc(const std::string & ModName, const FunctionDoc & doc);
    bool hasFunction(const std::string & name) const;
    Symbols::Value callFunction(const std::string & name, FunctionArguments & args) const;
    Symbols::Variables::Type getFunctionReturnType(const std::string & name);
    Symbols::Value getFunctionNullValue(const std::string & name);

    // --- Class registration ---
    bool hasClass(const std::string & className) const;
    void registerClass(const std::string & className);
    ClassInfo & getClassInfo(const std::string & className);
    void addProperty(const std::string & className, const std::string & propertyName, Symbols::Variables::Type type,
                    Parser::ParsedExpressionPtr defaultValueExpr = nullptr);
    void addMethod(const std::string & className, const std::string & methodName);
    void addMethod(const std::string & className, const std::string & methodName,
                  std::function<Symbols::Value(const std::vector<Symbols::Value> &)> cb,
                  const Symbols::Variables::Type & returnType = Symbols::Variables::Type::NULL_TYPE);
    bool hasProperty(const std::string & className, const std::string & propertyName) const;
    bool hasMethod(const std::string & className, const std::string & methodName) const;
    std::vector<std::string> getClassNames() const;

    // --- Object property management ---
    void setObjectProperty(const std::string& className, const std::string& propertyName, const Symbols::Value& value);
    Symbols::Value getObjectProperty(const std::string& className, const std::string& propertyName) const;
    void deleteObjectProperty(const std::string& className, const std::string& propertyName);
    bool hasObjectProperty(const std::string& className, const std::string& propertyName) const;
    void clearObjectProperties(const std::string& className);

    // --- Utility functions ---
    std::vector<std::string>  getFunctionNamesForModule(BaseModule * module) const;
    std::vector<std::string>  getPluginPaths() const;
    std::vector<BaseModule *> getPluginModules() const;
    BaseModule *              getCurrentModule() const;
    std::string               getCurrentModuleName() const;
    void generateMarkdownDocs(const std::string & outputDir) const;

    // Get the module for a class (for macro compatibility)
    BaseModule * getClassModule(const std::string & clsname) const;

    // Destructor
    ~UnifiedModuleManager();

  private:
    UnifiedModuleManager() = default;
    UnifiedModuleManager(const UnifiedModuleManager &) = delete;
    UnifiedModuleManager & operator=(const UnifiedModuleManager &) = delete;

    // Helper methods to reduce duplication
    template<typename T>
    const T& findOrThrow(const std::unordered_map<std::string, T>& map, 
                        const std::string& key, 
                        const std::string& errorMsg) const {
        auto it = map.find(key);
        if (it == map.end()) {
            throw std::runtime_error(errorMsg + ": " + key);
        }
        return it->second;
    }

    template<typename T>
    T& findOrThrow(std::unordered_map<std::string, T>& map, 
                   const std::string& key, 
                   const std::string& errorMsg) {
        auto it = map.find(key);
        if (it == map.end()) {
            throw std::runtime_error(errorMsg + ": " + key);
        }
        return it->second;
    }

    // Helper for module name normalization
    static std::string normalizeModuleName(const std::string& name) {
        if (name.rfind("lib", 0) == 0) {
            return name.substr(3);
        }
        return name;
    }

    // --- Module management ---
    struct PluginInfo {
        void* handle;
        std::string path;
        BaseModule* module;
    };
    std::vector<std::unique_ptr<BaseModule>> modules_;
    std::vector<PluginInfo> plugins_;
    BaseModule* currentModule_ = nullptr;

    // --- Registry entries ---
    struct RegistryEntry {
        BaseModule* module;
        FunctionDoc doc;
    };

    struct FunctionEntry : public RegistryEntry {
        std::shared_ptr<CallbackFunction> callback;
        Symbols::Variables::Type returnType;
    };

    struct ClassEntry : public RegistryEntry {
        ClassInfo info;
    };

    // --- Registries ---
    std::unordered_map<std::string, FunctionEntry> functions_;
    std::unordered_map<std::string, ClassEntry> classes_;

    // Helper for documentation generation
    void writeDoc(std::ofstream& file, 
                 const std::string& name,
                 const RegistryEntry& entry,
                 const std::string& prefix,
                 const Symbols::Variables::Type& returnType) const;
};

}  // namespace Modules

#define REGISTER_FUNCTION(fnName, retType, paramListVec, docStr, lambda)            \
    do {                                                                            \
        UnifiedModuleManager::instance().registerFunction(fnName, lambda, retType); \
        UnifiedModuleManager::instance().registerDoc(                               \
            Modules::UnifiedModuleManager::instance().getCurrentModule()->name(),   \
            FunctionDoc{ fnName, retType, paramListVec, docStr });                  \
    } while (0)

#define REGISTER_CLASS(className) UnifiedModuleManager::instance().registerClass(className)

#define REGISTER_METHOD(className, methodName, paramList, callback, retType, docStr)          \
    do {                                                                                      \
        std::string fullMethodName = std::string(className) + "::" + std::string(methodName);\
        UnifiedModuleManager::instance().registerFunction(fullMethodName, callback, retType); \
        UnifiedModuleManager::instance().addMethod(className, methodName, callback, retType); \
        UnifiedModuleManager::instance().registerDoc(                                         \
            Modules::UnifiedModuleManager::instance().getCurrentModule()->name(),             \
            FunctionDoc{ fullMethodName, retType, paramList, docStr });                      \
    } while (0)

#define REGISTER_PROPERTY(className, propertyName, type, defaultValue) \
    UnifiedModuleManager::instance().addProperty(className, propertyName, type, defaultValue)

#endif  // MODULES_UNIFIEDMODULEMANAGER_HPP
