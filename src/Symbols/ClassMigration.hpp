#ifndef SYMBOLS_CLASS_MIGRATION_HPP
#define SYMBOLS_CLASS_MIGRATION_HPP

#include "Symbols/ClassContainer.hpp"
#include "Symbols/ClassRegistry.hpp"
#include <string>
#include <iostream>

namespace Symbols {

/**
 * @brief Utilities for migrating classes from the old system to the new one.
 */
class ClassMigration {
public:
    /**
     * @brief Migrate a class from the old system to the new one.
     * @param className Name of the class to migrate.
     * @param registry Reference to the class registry.
     * @return True if the migration was successful, false otherwise.
     */
    static bool migrateClass(const std::string& className, ClassRegistry& registry) {
        try {
            // Get the class info from the old system
            const auto& oldClassInfo = ClassContainer::instance()->getClassInfo(className);
            
            // Register the class in the new system
            UnifiedClassContainer::ClassInfo& newClassInfo = registry.registerClass(
                oldClassInfo.name, oldClassInfo.parentClass, oldClassInfo.module);
            
            // Migrate properties
            for (const auto& prop : oldClassInfo.properties) {
                registry.getClassContainer().addProperty(
                    className, prop.name, prop.type, prop.isPrivate, nullptr);
            }
            
            // Migrate methods
            for (const auto& method : oldClassInfo.methods) {
                std::vector<UnifiedClassContainer::ParameterInfo> params;
                for (const auto& param : method.parameters) {
                    params.push_back(UnifiedClassContainer::ParameterInfo{
                        .name = param.name,
                        .type = param.type
                    });
                }
                
                registry.getClassContainer().addMethod(
                    className, method.name, method.returnType, params, method.isPrivate);
            }
            
            // Migrate static properties
            for (const auto& [propName, value] : oldClassInfo.objectProperties) {
                registry.setStaticProperty(className, propName, value);
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error migrating class " << className << ": " << e.what() << std::endl;
            return false;
        }
    }
    
    /**
     * @brief Migrate all classes from the old system to the new one.
     * @param registry Reference to the class registry.
     * @return Number of classes successfully migrated.
     */
    static int migrateAllClasses(ClassRegistry& registry) {
        int count = 0;
        
        // Get all class names from the old system
        auto classNames = ClassContainer::instance()->getClassNames();
        
        // Migrate each class
        for (const auto& className : classNames) {
            if (migrateClass(className, registry)) {
                count++;
            }
        }
        
        return count;
    }
};

} // namespace Symbols

#endif // SYMBOLS_CLASS_MIGRATION_HPP
