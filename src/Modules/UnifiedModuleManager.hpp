#ifndef MODULES_UNIFIEDMODULEMANAGER_HPP
#define MODULES_UNIFIEDMODULEMANAGER_HPP

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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

using FunctionArguments = const std::vector<Symbols::Value>;
using CallbackFunction = std::function<Symbols::Value(FunctionArguments&)>;


struct ClassInfo {
    struct PropertyInfo {
        std::string name;
        Symbols::Variables::Type type;
        Parser::ParsedExpressionPtr defaultValueExpr;
    };

    std::vector<PropertyInfo> properties;
    std::vector<std::string> methodNames;
};

class UnifiedModuleManager : public IModuleContext {
public:
    static UnifiedModuleManager& instance() {
        static UnifiedModuleManager mgr;
        return mgr;
    }

    // Module management functions
    void addModule(std::unique_ptr<BaseModule> module);
    void registerAll();
    void loadPlugins(const std::string& directory);
    void loadPlugin(const std::string& path);

    // Function registration

    void registerFunction(const std::string& name, CallbackFunction cb,
                         const Symbols::Variables::Type& returnType = Symbols::Variables::Type::NULL_TYPE);
    void registerDoc(const std::string& modName, const FunctionDoc& doc);
    bool hasFunction(const std::string& name) const;
    Symbols::Value callFunction(const std::string& name, FunctionArguments& args) const;
    Symbols::Variables::Type getFunctionReturnType(const std::string& name);
    Symbols::Value getFunctionNullValue(const std::string& name);

    // Class registration
    bool hasClass(const std::string& className) const;
    void registerClass(const std::string& className);
    ClassInfo& getClassInfo(const std::string& className);
    void addProperty(const std::string& className, const std::string& propertyName,
                    Symbols::Variables::Type type, Parser::ParsedExpressionPtr defaultValueExpr = nullptr);
    void addMethod(const std::string& className, const std::string& methodName);
    void addMethod(const std::string& className, const std::string& methodName,
                  std::function<Symbols::Value(const std::vector<Symbols::Value>&)> cb,
                  const Symbols::Variables::Type& returnType = Symbols::Variables::Type::NULL_TYPE);
    bool hasProperty(const std::string& className, const std::string& propertyName) const;
    bool hasMethod(const std::string& className, const std::string& methodName) const;
    std::vector<std::string> getClassNames() const;

    // Utility functions
    std::vector<std::string> getFunctionNamesForModule(BaseModule* module) const;
    std::vector<std::string> getPluginPaths() const;
    std::vector<BaseModule*> getPluginModules() const;
    BaseModule* getCurrentModule() const;
    std::string getCurrentModuleName() const;

private:
    UnifiedModuleManager() = default;
    ~UnifiedModuleManager();

    // Module management
    std::vector<std::unique_ptr<BaseModule>> modules_;
    std::vector<void*> pluginHandles_;
    std::vector<BaseModule*> pluginModules_;
    std::vector<std::string> pluginPaths_;
    BaseModule* currentModule_ = nullptr;

    // Function registry

    struct FunctionEntry {
        CallbackFunction callback;
        Symbols::Variables::Type returnType;
        BaseModule* module;
        FunctionDoc doc;
    };
    std::unordered_map<std::string, FunctionEntry> functions_;

    // Class registry
    struct ClassEntry {
        ClassInfo info;
        BaseModule* module;
    };
    std::unordered_map<std::string, ClassEntry> classes_;
};

} // namespace Modules

#define REGISTER_FUNCTION_WITH_DOC(context, modName, fnName, retType, paramListVec, docStr, lambda) \
    do {                                                                                          \
        (context).registerFunction(fnName, lambda, retType);                                        \
        (context).registerDoc(modName, FunctionDoc{ fnName, retType, paramListVec, docStr });       \
    } while (0)

#define REGISTER_CLASS(className) \
    UnifiedModuleManager::instance().registerClass(className)

#define REGISTER_METHOD(className, methodName, callback, retType, docStr) \
    do { \
        UnifiedModuleManager::instance().addMethod(className, methodName, callback, retType); \
        REGISTER_FUNCTION_WITH_DOC(UnifiedModuleManager::instance(), #className, methodName, retType, {}, docStr, callback); \
    } while (0)

#define REGISTER_PROPERTY(className, propertyName, type, defaultValue) \
    UnifiedModuleManager::instance().addProperty(className, propertyName, type, defaultValue)

#endif // MODULES_UNIFIEDMODULEMANAGER_HPP
