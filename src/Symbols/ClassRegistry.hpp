 #ifndef SYMBOLS_CLASS_REGISTRY_HPP
 #define SYMBOLS_CLASS_REGISTRY_HPP

 #include <string>
 #include <unordered_map>
 #include <vector>
#include "Parser/ParsedExpression.hpp"
#include "Symbols/VariableTypes.hpp"
#include "Modules/ModuleManager.hpp"

 namespace Symbols {

 // Information about a class: its properties and methods
 struct ClassInfo {
     struct PropertyInfo {
         std::string name;
         Variables::Type type;
         Parser::ParsedExpressionPtr defaultValueExpr; // nullptr if none
     };
     std::vector<PropertyInfo> properties;
     std::vector<std::string> methodNames;
 };

 // Registry of classes defined in the program
 class ClassRegistry {
   public:
    /**
     * @brief Get module that registered the given class, or nullptr.
     */
    Modules::BaseModule * getClassModule(const std::string & className) const;
    /**
     * @brief Get module that registered the given method, or nullptr.
     */
    Modules::BaseModule * getMethodModule(const std::string & className,
                                         const std::string & methodName) const;
     // Get the singleton instance
     static ClassRegistry & instance();

    // Check if a class is registered
    bool hasClass(const std::string & className) const;
    // Register a new class by name
    void registerClass(const std::string & className);
    // Get ClassInfo for a registered class
    ClassInfo & getClassInfo(const std::string & className);
    
    // Register a new property for a class
    // defaultValueExpr may be nullptr if no default value is provided
    void addProperty(const std::string & className,
                     const std::string & propertyName,
                     Variables::Type type,
                     Parser::ParsedExpressionPtr defaultValueExpr = nullptr);
    
    // Register a new method for a class
    void addMethod(const std::string & className,
                   const std::string & methodName);
    
    // Check if a property exists in the class
    bool hasProperty(const std::string & className,
                     const std::string & propertyName) const;
    
    // Check if a method exists in the class
    bool hasMethod(const std::string & className,
                   const std::string & methodName) const;
    
    // Get all registered class names
    std::vector<std::string> getClassNames() const;

   private:
     std::unordered_map<std::string, ClassInfo>                     classes_;
    // Mapping from class name to module that registered it
    std::unordered_map<std::string, Modules::BaseModule *>          classModuleMap_;
    // Mapping from fully-qualified method (Class::Method) to registering module
    std::unordered_map<std::string, Modules::BaseModule *>          methodModuleMap_;
 };

} // namespace Symbols

 #endif // SYMBOLS_CLASS_REGISTRY_HPP