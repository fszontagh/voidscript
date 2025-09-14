#include "SymbolContainer.hpp" 
#include "Parser/ParsedExpression.hpp"  // For implementation methods using ParsedExpressionPtr
#include "Modules/BaseModule.hpp"  // For module implementation methods

namespace Modules {
    void BaseModuleDeleter::operator()(BaseModule* module) const {
        delete module;
    }

    BaseModulePtr make_base_module_ptr(std::unique_ptr<BaseModule> module) {
        return BaseModulePtr(module.release());
    }
}

namespace Symbols {
    std::string SymbolContainer::initial_scope_name_for_singleton_;
    bool SymbolContainer::is_initialized_for_singleton_ = false;

    SymbolContainer::SymbolContainer(const std::string & default_scope_name) {
        if (default_scope_name.empty()) {
            throw std::runtime_error("SymbolContainer default scope name cannot be empty during construction.");
        }
        create(default_scope_name);  // Creates and enters the initial scope
    }

    SymbolContainer::~SymbolContainer() = default;

    void SymbolContainer::create(const std::string & name) {
        scopes_[name] = std::make_shared<SymbolTable>(SCOPE_SEPARATOR);
        scopeStack_.push_back(name);
    }

    void SymbolContainer::enter(const std::string & name) {
        auto it = scopes_.find(name);
        if (it != scopes_.end()) {
            scopeStack_.push_back(name);
        } else {
            throw std::runtime_error("Scope does not exist: " + name);
        }
    }

    void SymbolContainer::enterPreviousScope() {
        if (scopeStack_.size() > 1) {
            scopeStack_.pop_back();
        }
    }

    void SymbolContainer::validateAndCleanupScopeStack(const std::string & expectedScope) {
        // Find the expected scope in the stack
        auto it = std::find(scopeStack_.rbegin(), scopeStack_.rend(), expectedScope);
        if (it != scopeStack_.rend()) {
            // Remove everything above it
            auto forward_it = it.base() - 1;
            scopeStack_.erase(forward_it, scopeStack_.end());
        } else {
            if (!scopeStack_.empty()) {
                scopeStack_.pop_back();
            }
        }
    }

    bool SymbolContainer::enterPreviousScopeWithValidation(const std::string & expectedCurrentScope) {
        if (scopeStack_.size() <= 1) {
            return false;
        }
        if (!expectedCurrentScope.empty() && currentScopeName() != expectedCurrentScope) {
            return false;
        }
        scopeStack_.pop_back();
        return true;
    }

    std::string SymbolContainer::currentScopeName() const {
        if (scopeStack_.empty()) {
            return std::string();
        }
        return scopeStack_.back();
    }

    std::vector<std::string> SymbolContainer::getScopeNames() const {
        std::vector<std::string> result;
        result.reserve(scopes_.size());
        for (const auto & [scopeName, _] : scopes_) {
            result.push_back(scopeName);
        }
        return result;
    }

    const std::vector<std::string>& SymbolContainer::getScopeStack() const {
        return scopeStack_;
    }

    std::string SymbolContainer::enterFunctionCallScope(const std::string & baseFunctionScopeName) {
        unsigned long long call_id = next_call_frame_id_++;
        std::string callScopeName = baseFunctionScopeName + Symbols::SymbolContainer::CALL_SCOPE + std::to_string(call_id);
        create(callScopeName);
        return callScopeName;
    }

    std::string SymbolContainer::add(const SymbolPtr & symbol) {
        switch (symbol->kind()) {
            case Symbols::Kind::Variable:
                return addVariable(symbol);
            case Symbols::Kind::Function:
                return addFunction(symbol);
            case Symbols::Kind::Method:
                return addMethod(symbol);
            case Symbols::Kind::Class:
                return addClass(symbol);
            case Symbols::Kind::Constant:
                return addConstant(symbol);
            case Symbols::Kind::ENUM:
                return addEnum(symbol);
            default:
                const std::string ns = getNamespaceForSymbol(symbol);
                scopes_[currentScopeName()]->define(ns, symbol);
                return ns;
        }
    }

    std::string SymbolContainer::addFunction(const SymbolPtr & function) {
        if (function->kind() != Symbols::Kind::Function) {
            throw std::runtime_error("Symbol must be a function to use addFunction");
        }
        const std::string ns = DEFAULT_FUNCTIONS_SCOPE;
        scopes_[currentScopeName()]->define(ns, function);
        return ns;
    }

    std::string SymbolContainer::addFunction(const SymbolPtr & function, const std::string & scopeName) {
        if (function->kind() != Symbols::Kind::Function) {
            throw std::runtime_error("Symbol must be a function to use addFunction");
        }
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            throw std::runtime_error("Cannot define function in non-existent scope: " + scopeName);
        }
        const std::string ns = DEFAULT_FUNCTIONS_SCOPE;
        it->second->define(ns, function);
        return ns;
    }

    std::string SymbolContainer::addMethod(const SymbolPtr & method) {
        if (method->kind() != Symbols::Kind::Function && method->kind() != Symbols::Kind::Method) {
            throw std::runtime_error("Symbol must be a function or method to use addMethod");
        }
        const std::string ns = METHOD_SCOPE;
        scopes_[currentScopeName()]->define(ns, method);
        return ns;
    }

    std::string SymbolContainer::addMethod(const SymbolPtr & method, const std::string & scopeName) {
        if (method->kind() != Symbols::Kind::Function && method->kind() != Symbols::Kind::Method) {
            throw std::runtime_error("Symbol must be a function or method to use addMethod");
        }
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            throw std::runtime_error("Cannot define method in non-existent scope: " + scopeName);
        }
        const std::string ns = METHOD_SCOPE;
        it->second->define(ns, method);
        return ns;
    }

    std::string SymbolContainer::addVariable(const SymbolPtr & variable) {
        if (variable->kind() != Symbols::Kind::Variable) {
            throw std::runtime_error("Symbol must be a variable to use addVariable");
        }
        const std::string current_scope = currentScopeName();
        const std::string ns = DEFAULT_VARIABLES_SCOPE;
        scopes_[current_scope]->define(ns, variable);
        return ns;
    }

    std::string SymbolContainer::addVariable(const SymbolPtr & variable, const std::string & scopeName) {
        if (variable->kind() != Symbols::Kind::Variable) {
            throw std::runtime_error("Symbol must be a variable to use addVariable");
        }
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            throw std::runtime_error("Cannot define variable in non-existent scope: " + scopeName);
        }
        const std::string ns = DEFAULT_VARIABLES_SCOPE;
        it->second->define(ns, variable);
        return ns;
    }

    std::string SymbolContainer::addConstant(const SymbolPtr & constant) {
        if (constant->kind() != Symbols::Kind::Constant) {
            throw std::runtime_error("Symbol must be a constant to use addConstant");
        }
        const std::string ns = DEFAULT_CONSTANTS_SCOPE;
        scopes_[currentScopeName()]->define(ns, constant);
        return ns;
    }

    std::string SymbolContainer::addConstant(const SymbolPtr & constant, const std::string & scopeName) {
        if (constant->kind() != Symbols::Kind::Constant) {
            throw std::runtime_error("Symbol must be a constant to use addConstant");
        }
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            throw std::runtime_error("Cannot define constant in non-existent scope: " + scopeName);
        }
        const std::string ns = DEFAULT_CONSTANTS_SCOPE;
        it->second->define(ns, constant);
        return ns;
    }

    std::string SymbolContainer::addClass(const SymbolPtr & classSymbol) {
        if (classSymbol->kind() != Symbols::Kind::Class) {
            throw std::runtime_error("Symbol must be a class to use addClass");
        }
        const std::string ns = DEFAULT_VARIABLES_SCOPE;
        scopes_[currentScopeName()]->define(ns, classSymbol);
        return ns;
    }

    std::string SymbolContainer::addClass(const SymbolPtr & classSymbol, const std::string & scopeName) {
        if (classSymbol->kind() != Symbols::Kind::Class) {
            throw std::runtime_error("Symbol must be a class to use addClass");
        }
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            throw std::runtime_error("Cannot define class in non-existent scope: " + scopeName);
        }
        const std::string ns = DEFAULT_VARIABLES_SCOPE;
        it->second->define(ns, classSymbol);
        return ns;
    }

    std::string SymbolContainer::addEnum(const SymbolPtr & enumSymbol) {
        if (enumSymbol->kind() != Symbols::Kind::ENUM) {
            throw std::runtime_error("Symbol must be an enum to use addEnum");
        }
        const std::string ns = DEFAULT_VARIABLES_SCOPE;
        scopes_[currentScopeName()]->define(ns, enumSymbol);
        return ns;
    }

    std::string SymbolContainer::addEnum(const SymbolPtr & enumSymbol, const std::string & scopeName) {
        if (enumSymbol->kind() != Symbols::Kind::ENUM) {
            throw std::runtime_error("Symbol must be an enum to use addEnum");
        }
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            throw std::runtime_error("Cannot define enum in non-existent scope: " + scopeName);
        }
        const std::string ns = DEFAULT_VARIABLES_SCOPE;
        it->second->define(ns, enumSymbol);
        return ns;
    }

    std::vector<SymbolPtr> SymbolContainer::getAll(const std::string & ns) const {
        std::vector<SymbolPtr> result;
        for (const auto & [_, table] : scopes_) {
            auto symbols = ns.empty() ? table->listAll() : table->listAll(ns);
            result.insert(result.end(), symbols.begin(), symbols.end());
        }
        return result;
    }

    bool SymbolContainer::exists(const std::string & name, std::string fullNamespace) const {
        if (fullNamespace.empty()) {
            fullNamespace = currentScopeName();
        }
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const auto & scopeName = *it;
            auto tableIt = scopes_.find(scopeName);
            if (tableIt != scopes_.end() && tableIt->second->exists(fullNamespace, name)) {
                return true;
            }
        }
        return false;
    }

    SymbolPtr SymbolContainer::get(const std::string & fullNamespace, const std::string & name) const {
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const auto & scopeName = *it;
            auto tableIt = scopes_.find(scopeName);
            if (tableIt != scopes_.end()) {
                auto sym = tableIt->second->get(fullNamespace, name);
                if (sym) {
                    return sym;
                }
            }
        }
        return nullptr;
    }

    SymbolPtr SymbolContainer::findSymbol(const std::string & name) {
        if (SymbolPtr variable = getVariable(name)) {
            return variable;
        }
        if (SymbolPtr constant = getConstant(name)) {
            return constant;
        }
        if (SymbolPtr function = getFunction(name)) {
            return function;
        }
        if (SymbolPtr method = getMethod(name)) {
            return method;
        }
        return nullptr;
    }

    std::string SymbolContainer::findClassNamespace(const std::string & className) {
        for (const auto & [scopeName, table] : scopes_) {
            auto classSymbol = table->get(DEFAULT_VARIABLES_SCOPE, className);
            if (classSymbol && classSymbol->getKind() == Kind::Class) {
                return scopeName;
            }
        }
        return "";
    }

    SymbolPtr SymbolContainer::findMethod(const std::string & className, const std::string & methodName) {
        if (hasClass(className)) {
            const ClassInfo & classInfo = getClassInfo(className);
            for (const auto & method : classInfo.methods) {
                if (method.name == methodName) {
                    static SymbolPtr dummySymbol = nullptr;
                    if (!dummySymbol) {
                        dummySymbol = std::make_shared<Symbols::VariableSymbol>("dummy", Symbols::ValuePtr::null(), "", Variables::Type::NULL_TYPE);
                    }
                    return dummySymbol;
                }
            }
        }
        std::string classScope = findClassNamespace(className);
        if (classScope.empty()) {
            return nullptr;
        }
        std::string classMethodScope = classScope + SCOPE_SEPARATOR + className;
        auto scopeTable = getScopeTable(classMethodScope);
        if (!scopeTable) {
            return nullptr;
        }
        auto result = scopeTable->get(METHOD_SCOPE, methodName);
        return result;
    }

    std::vector<Symbols::FunctionParameterInfo> SymbolContainer::getNativeMethodParameters(const std::string & className, const std::string & methodName) {
        if (hasClass(className)) {
            const ClassInfo & classInfo = getClassInfo(className);
            for (const auto & method : classInfo.methods) {
                if (method.name == methodName) {
                    return method.parameters;
                }
            }
        }
        return {};
    }

    SymbolPtr SymbolContainer::getFunction(const std::string & name) const {
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const std::string & scopeName = *it;
            auto tableIt = scopes_.find(scopeName);
            if (tableIt != scopes_.end()) {
                auto func = tableIt->second->get(DEFAULT_FUNCTIONS_SCOPE, name);
                if (func && func->getKind() == Kind::Function) {
                    return func;
                }
            }
        }
        return nullptr;
    }

    SymbolPtr SymbolContainer::getFunction(const std::string & scopeName, const std::string & name) const {
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            return nullptr;
        }
        auto func = it->second->get(DEFAULT_FUNCTIONS_SCOPE, name);
        if (func && func->getKind() == Kind::Function) {
            return func;
        }
        return nullptr;
    }

    SymbolPtr SymbolContainer::getMethod(const std::string & name) const {
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const std::string & scopeName = *it;
            auto tableIt = scopes_.find(scopeName);
            if (tableIt != scopes_.end()) {
                auto method = tableIt->second->get(METHOD_SCOPE, name);
                if (method && method->getKind() == Kind::Function) {
                    return method;
                }
            }
        }
        return nullptr;
    }

    SymbolPtr SymbolContainer::getMethod(const std::string & scopeName, const std::string & name) const {
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            return nullptr;
        }
        auto method = it->second->get(METHOD_SCOPE, name);
        if (method && method->getKind() == Kind::Function) {
            return method;
        }
        return nullptr;
    }

    SymbolPtr SymbolContainer::getVariable(const std::string & name) const {
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const auto & scopeName = *it;
            auto tableIt = scopes_.find(scopeName);
            if (tableIt != scopes_.end()) {
                auto variable = tableIt->second->get(DEFAULT_VARIABLES_SCOPE, name);
                if (variable && variable->getKind() == Kind::Variable) {
                    return variable;
                }
            }
        }
        return nullptr;
    }

    SymbolPtr SymbolContainer::getVariable(const std::string & scopeName, const std::string & name) const {
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            return nullptr;
        }
        auto variable = it->second->get(DEFAULT_VARIABLES_SCOPE, name);
        if (variable) {
            if (variable->getKind() == Kind::Variable) {
                return variable;
            }
        }
        return nullptr;
    }

    SymbolPtr SymbolContainer::getConstant(const std::string & name) const {
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const auto & scopeName = *it;
            auto tableIt = scopes_.find(scopeName);
            if (tableIt != scopes_.end()) {
                auto constant = tableIt->second->get(DEFAULT_CONSTANTS_SCOPE, name);
                if (constant && constant->getKind() == Kind::Constant) {
                    return constant;
                }
            }
        }
        return nullptr;
    }

    SymbolPtr SymbolContainer::getConstant(const std::string & scopeName, const std::string & name) const {
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            return nullptr;
        }
        auto constant = it->second->get(DEFAULT_CONSTANTS_SCOPE, name);
        if (constant && constant->getKind() == Kind::Constant) {
            return constant;
        }
        return nullptr;
    }

    SymbolPtr SymbolContainer::getEnum(const std::string & name) const {
        for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
            const auto & scopeName = *it;
            auto tableIt = scopes_.find(scopeName);
            if (tableIt != scopes_.end()) {
                auto enumSymbol = tableIt->second->get(DEFAULT_VARIABLES_SCOPE, name);
                if (enumSymbol && enumSymbol->getKind() == Kind::ENUM) {
                    return enumSymbol;
                }
            }
        }
        return nullptr;
    }

    SymbolPtr SymbolContainer::getEnum(const std::string & scopeName, const std::string & name) const {
        auto it = scopes_.find(scopeName);
        if (it == scopes_.end()) {
            return nullptr;
        }
        auto enumSymbol = it->second->get(DEFAULT_VARIABLES_SCOPE, name);
        if (enumSymbol && enumSymbol->getKind() == Kind::ENUM) {
            return enumSymbol;
        }
        return nullptr;
    }

    std::string SymbolContainer::dump() {
        std::string result;
        result += "\n--- Symbol Table Dump ---\n";
        for (const auto & scope_name : instance()->getScopeNames()) {
            result += "Scope: '" + scope_name + "'\n";
            auto tablePtr = instance()->getScopeTable(scope_name);
            if (tablePtr) {
                for (const auto & symbol : tablePtr->listAll()) {
                    result += symbol->dump() + '\n';
                    dumpValue(symbol->getValue(), result, 2);
                }
            } else {
                result += "\t(Error: Scope table not found)\n";
            }
        }
        result += "--- End Dump ---\n";
        return result;
    }

    std::shared_ptr<SymbolTable> SymbolContainer::getScopeTable(const std::string & scopeName) const {
        auto it = scopes_.find(scopeName);
        if (it != scopes_.end()) {
            return it->second;
        }
        return nullptr;
    }

    ClassInfo & SymbolContainer::registerClass(const std::string & className, Modules::BaseModule * module) {
        if (hasClass(className)) {
            throw std::runtime_error("Class already registered: " + className);
        }
        ClassInfo classInfo;
        classInfo.name = className;
        classInfo.module = module;
        classes_[className] = classInfo;
        return classes_[className];
    }

    ClassInfo & SymbolContainer::registerClass(const std::string & className, const std::string & parentClassName, Modules::BaseModule * module) {
        if (hasClass(className)) {
            throw std::runtime_error("Class already registered: " + className);
        }
        if (!hasClass(parentClassName)) {
            throw std::runtime_error("Parent class not found: " + parentClassName);
        }
        ClassInfo classInfo;
        classInfo.name = className;
        classInfo.parentClass = parentClassName;
        classInfo.module = module;
        classes_[className] = classInfo;
        return classes_[className];
    }

    bool SymbolContainer::hasClass(const std::string & className) const {
        static thread_local int recursionDepth = 0;
        static thread_local std::unordered_set<std::string> visitedClasses;
        if (recursionDepth > 10) {
            return false;
        }
        if (visitedClasses.find(className) != visitedClasses.end()) {
            return false;
        }
        recursionDepth++;
        visitedClasses.insert(className);
        bool result = classes_.find(className) != classes_.end();
        visitedClasses.erase(className);
        recursionDepth--;
        return result;
    }

    ClassInfo & SymbolContainer::getClassInfo(const std::string & className) {
        auto it = classes_.find(className);
        if (it == classes_.end()) {
            throw std::runtime_error("Class not found: " + className);
        }
        return it->second;
    }

    const ClassInfo & SymbolContainer::getClassInfo(const std::string & className) const {
        auto it = classes_.find(className);
        if (it == classes_.end()) {
            throw std::runtime_error("Class not found: " + className);
        }
        return it->second;
    }

    void SymbolContainer::addProperty(const std::string & className, const std::string & propertyName, Variables::Type type, bool isPrivate, Parser::ParsedExpressionPtr defaultValueExpr) {
        ClassInfo & classInfo = getClassInfo(className);
        for (const auto & prop : classInfo.properties) {
            if (prop.name == propertyName) {
                throw std::runtime_error("Property already exists in class: " + className + "::" + propertyName);
            }
        }
        PropertyInfo propertyInfo;
        propertyInfo.name = propertyName;
        propertyInfo.type = type;
        propertyInfo.isPrivate = isPrivate;
        propertyInfo.defaultValueExpr = defaultValueExpr;
        classInfo.properties.push_back(propertyInfo);
    }

    void SymbolContainer::addMethod(const std::string & className, const std::string & methodName, Variables::Type returnType, std::vector<FunctionParameterInfo> parameters, bool isPrivate) {
        ClassInfo & classInfo = getClassInfo(className);
        for (const auto & method : classInfo.methods) {
            if (method.name == methodName) {
                throw std::runtime_error("Method already exists in class: " + className + "::" + methodName);
            }
        }
        MethodInfo methodInfo;
        methodInfo.name = methodName;
        methodInfo.qualifiedName = className + SCOPE_SEPARATOR + methodName;
        methodInfo.returnType = returnType;
        methodInfo.parameters = parameters;
        methodInfo.isPrivate = isPrivate;
        FunctionDoc doc;
        doc.name = methodInfo.qualifiedName;
        doc.returnType = returnType;
        doc.parameterList = parameters;
        methodInfo.documentation = doc;
        classInfo.methods.push_back(methodInfo);
    }

    void SymbolContainer::addNativeMethod(const std::string & className, const std::string & methodName, std::function<ValuePtr(const std::vector<ValuePtr> &)> implementation, Variables::Type returnType, std::vector<FunctionParameterInfo> parameters, bool isPrivate, const std::string & description) {
        ClassInfo & classInfo = getClassInfo(className);
        for (const auto & method : classInfo.methods) {
            if (method.name == methodName) {
                throw std::runtime_error("Method already exists in class: " + className + "::" + methodName);
            }
        }
        MethodInfo methodInfo;
        methodInfo.name = methodName;
        methodInfo.qualifiedName = className + SCOPE_SEPARATOR + methodName;
        methodInfo.returnType = returnType;
        methodInfo.parameters = parameters;
        methodInfo.isPrivate = isPrivate;
        methodInfo.nativeImplementation = implementation;
        FunctionDoc doc;
        doc.name = methodInfo.qualifiedName;
        doc.returnType = returnType;
        doc.parameterList = parameters;
        doc.description = description;
        methodInfo.documentation = doc;
        classInfo.methods.push_back(methodInfo);
    }

    bool SymbolContainer::hasProperty(const std::string & className, const std::string & propertyName) const {
        if (!hasClass(className)) {
            return false;
        }
        const ClassInfo & classInfo = getClassInfo(className);
        for (const auto & prop : classInfo.properties) {
            if (prop.name == propertyName) {
                return true;
            }
        }
        if (!classInfo.parentClass.empty()) {
            return hasProperty(classInfo.parentClass, propertyName);
        }
        return false;
    }

    bool SymbolContainer::hasMethod(const std::string & className, const std::string & methodName) const {
        std::unordered_set<std::string> visited;
        return hasMethodInternal(className, methodName, visited, 0);
    }

    bool SymbolContainer::hasMethodInternal(const std::string & className, const std::string & methodName, std::unordered_set<std::string>& visited, int depth) const {
        const int MAX_RECURSION_DEPTH = 10;
        if (depth > MAX_RECURSION_DEPTH) {
            return false;
        }
        if (visited.find(className) != visited.end()) {
            return false;
        }
        visited.insert(className);
        if (!hasClass(className)) {
            return false;
        }
        const ClassInfo & classInfo = getClassInfo(className);
        for (const auto & method : classInfo.methods) {
            if (method.name == methodName) {
                return true;
            }
        }
        if (!classInfo.parentClass.empty()) {
            return hasMethodInternal(classInfo.parentClass, methodName, visited, depth + 1);
        }
        return false;
    }

    std::vector<std::string> SymbolContainer::getClassNames() const {
        std::vector<std::string> names;
        names.reserve(classes_.size());
        for (const auto & [name, _] : classes_) {
            names.push_back(name);
        }
        return names;
    }

    std::string SymbolContainer::getNamespaceForSymbol(const SymbolPtr & symbol) const {
        switch (symbol->kind()) {
            case Kind::Variable: return DEFAULT_VARIABLES_SCOPE;
            case Kind::Constant: return DEFAULT_CONSTANTS_SCOPE;
            case Kind::Function: return DEFAULT_FUNCTIONS_SCOPE;
            case Kind::Method: return METHOD_SCOPE;
            default: return "";
        }
    }

    void SymbolContainer::dumpValue(const ValuePtr & value, std::string & result, int indent) {
        if (!value || value->isNull()) {
            return;
        }
        std::string indentStr(indent * 2, ' ');
        result += indentStr + "Value: " + value->toString() + " (Type: " + Variables::TypeToString(value->getType()) + ")\n";
        if (value->getType() == Variables::Type::OBJECT || value->getType() == Variables::Type::CLASS) {
            const auto & objMap = value->getObject();
            for (const auto & [key, val] : objMap) {
                result += indentStr + "  Property '" + key + "':\n";
                dumpValue(val, result, indent + 1);
            }
        }
    }

    std::vector<std::string> SymbolContainer::getModuleNames() const {
        std::vector<std::string> names;
        names.reserve(modules_.size());
        for (const auto & [name, _] : modules_) {
            names.push_back(name);
        }
        return names;
    }

    std::vector<std::string> SymbolContainer::getBuiltInModuleNames() const {
        std::vector<std::string> names;
        for (const auto & [name, modulePtr] : modules_) {
            if (modulePtr->isBuiltIn()) {
                names.push_back(name);
            }
        }
        return names;
    }

    std::vector<std::string> SymbolContainer::getExternalModuleNames() const {
        std::vector<std::string> names;
        for (const auto & [name, modulePtr] : modules_) {
            if (!modulePtr->isBuiltIn()) {
                names.push_back(name);
            }
        }
        return names;
    }

    std::string SymbolContainer::getModuleDescription(const std::string & moduleName) const {
        auto it = moduleDescriptions_.find(moduleName);
        if (it != moduleDescriptions_.end()) {
            return it->second;
        }
        return "";
    }

    void SymbolContainer::registerModule(Modules::BaseModulePtr module) {
        if (!module) {
            throw std::invalid_argument("Cannot register null module");
        }
        setCurrentModule(module.get());
        module->registerFunctions();
        storeModule(std::move(module));
        setCurrentModule(nullptr);
    }

    void SymbolContainer::storeModule(Modules::BaseModulePtr module) {
        if (!module) {
            throw std::invalid_argument("Cannot store null module");
        }
        std::string moduleName = module->name();
        if (moduleName.empty()) {
            throw std::invalid_argument("Module name cannot be empty");
        }
        std::string moduleDescription = module->description();
        if (!moduleDescription.empty()) {
            moduleDescriptions_[moduleName] = moduleDescription;
        }
        modules_[moduleName] = std::move(module);
    }

    void SymbolContainer::setCurrentModule(Modules::BaseModule * module) {
        currentModule_ = module;
    }

    Modules::BaseModule * SymbolContainer::getCurrentModule() const {
        return currentModule_;
    }

    bool SymbolContainer::hasModule(const std::string & moduleName) const {
        return modules_.find(moduleName) != modules_.end();
    }

    Modules::BaseModulePtr SymbolContainer::getModule(const std::string & moduleName) const {
        auto it = modules_.find(moduleName);
        if (it != modules_.end()) {
            return it->second;
        }
        return nullptr;
    }

} // namespace Symbols
