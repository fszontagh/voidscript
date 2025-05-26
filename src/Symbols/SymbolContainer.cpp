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

    void SymbolContainer::storeModule(Modules::BaseModulePtr module) {
        if (!module) {
            throw std::invalid_argument("Cannot store null module");
        }
        
        std::string moduleName = module->name();
        if (moduleName.empty()) {
            throw std::invalid_argument("Module name cannot be empty");
        }
        
        modules_[moduleName] = std::move(module);
    }

    void SymbolContainer::registerModule(Modules::BaseModulePtr module) {
        if (!module) {
            throw std::invalid_argument("Cannot register null module");
        }
        
        // Set as current module for registration
        setCurrentModule(module.get());
        
        // Register the module's functions
        module->registerFunctions();
        
        // Store the module by name
        storeModule(std::move(module));
        
        // Clear current module
        setCurrentModule(nullptr);
    }

} // namespace Symbols 