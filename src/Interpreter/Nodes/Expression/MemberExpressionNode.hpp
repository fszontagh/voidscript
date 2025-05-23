// filepath: /home/fszontagh/soniscript/src/Interpreter/Nodes/Expression/MemberExpressionNode.hpp
#ifndef INTERPRETER_MEMBER_EXPRESSION_NODE_HPP
#define INTERPRETER_MEMBER_EXPRESSION_NODE_HPP

#include <memory>
#include <string>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

// Expression node for member access: object->property
class MemberExpressionNode : public ExpressionNode {
  public:
    MemberExpressionNode(std::unique_ptr<ExpressionNode> objectExpr, std::string propertyName,
                         const std::string & filename, int line, size_t column) :
        objectExpr_(std::move(objectExpr)),
        propertyName_(std::move(propertyName)),
        filename_(filename),
        line_(line),
        column_(column) {}

    Symbols::ValuePtr evaluate(Interpreter & interpreter, std::string /*filename*/, int /*line*/,
                               size_t /*col */) const override {
        std::cout << "DEBUG: MemberExpressionNode::evaluate called for property: " << propertyName_ << std::endl;
        auto objVal = objectExpr_->evaluate(interpreter, filename_, line_, column_);
        std::cout << "DEBUG: Object evaluated, type: " << Symbols::Variables::TypeToString(objVal->getType()) << std::endl;

        // Allow member access on plain objects and class instances
        if (objVal->getType() != Symbols::Variables::Type::OBJECT &&
            objVal->getType() != Symbols::Variables::Type::CLASS) {
            throw Exception("Attempted to access member '" + propertyName_ + "' of non-object", filename_, line_,
                            column_);
        }

        Symbols::ObjectMap& map = objVal->get<Symbols::ObjectMap>();
        std::cout << "DEBUG: Got object map, keys: ";
        for (const auto& pair : map) {
            std::cout << pair.first << " ";
        }
        std::cout << std::endl;

        // Try both the property name as-is and with '$' prefix for class properties
        std::string propertyNameWithPrefix = propertyName_[0] == '$' ? propertyName_ : "$" + propertyName_;
        std::string propertyNameToUse = propertyName_;

        // Handle property lookup in class instances
        if (objVal->getType() == Symbols::Variables::Type::CLASS) {
            auto classIt = map.find("__class__");
            if (classIt != map.end() && classIt->second->getType() == Symbols::Variables::Type::STRING) {
                std::string className = classIt->second->get<std::string>();
                std::cout << "DEBUG: Class name from metadata: " << className << std::endl;
                auto &      manager   = Modules::UnifiedModuleManager::instance();
                
                if (manager.hasClass(className)) {
                    std::cout << "DEBUG: Class found in manager: " << className << std::endl;
                    
                    // First check if we need to try the propertyName with $ prefix
                    if (!manager.hasProperty(className, propertyName_) && manager.hasProperty(className, propertyNameWithPrefix)) {
                        std::cout << "DEBUG: Using property name with $ prefix: " << propertyNameWithPrefix << std::endl;
                        propertyNameToUse = propertyNameWithPrefix;
                    }
                    
                    // First try to find it as a method in the class
                    if (manager.hasMethod(className, propertyName_)) {
                        std::cout << "DEBUG: Property is a method: " << propertyName_ << std::endl;
                        // Just return the method name without qualification
                        // The method call expression will handle the full resolution
                        return Symbols::ValuePtr(propertyName_);
                    }
                    
                    // If not a method, check if it's a valid property (with or without the $ prefix)
                    bool hasPropertyWithoutPrefix = manager.hasProperty(className, propertyName_);
                    bool hasPropertyWithPrefix = manager.hasProperty(className, propertyNameWithPrefix);
                    std::cout << "DEBUG: Manager hasProperty(" << className << ", " << propertyName_ << "): " 
                              << (hasPropertyWithoutPrefix ? "true" : "false") << std::endl;
                    std::cout << "DEBUG: Manager hasProperty(" << className << ", " << propertyNameWithPrefix << "): " 
                              << (hasPropertyWithPrefix ? "true" : "false") << std::endl;
                    
                    if (!hasPropertyWithoutPrefix && !hasPropertyWithPrefix) {
                        throw Exception("Neither method nor property '" + propertyName_ + "' found in class '" + className + "'",
                                      filename_, line_, column_);
                    }
                } else {
                    std::cout << "DEBUG: Class NOT found in manager: " << className << std::endl;
                }
            } else {
                std::cout << "DEBUG: __class__ metadata not found or not a string" << std::endl;
            }
        }

        // First try with the property name as-is
        std::cout << "DEBUG: Looking for property '" << propertyNameToUse << "' in object map" << std::endl;
        auto it = map.find(propertyNameToUse);
        
        // If not found, try with the alternate name (with or without $ prefix)
        if (it == map.end()) {
            std::string altPropertyName = (propertyNameToUse == propertyName_) ? propertyNameWithPrefix : propertyName_;
            std::cout << "DEBUG: Not found, trying alternate property name: '" << altPropertyName << "'" << std::endl;
            it = map.find(altPropertyName);
        }
        
        if (it == map.end()) {
            throw Exception("Property '" + propertyName_ + "' not found in object", filename_, line_, column_);
        }
        
        // Check for null property value
        if (!it->second) {
            throw Exception("Property '" + propertyName_ + "' points to a null value pointer", filename_, line_, column_);
        }
        
        if (it->second->isNULL()) {
            throw Exception("Property '" + propertyName_ + "' is null in MemberExpressionNode", filename_, line_, column_);
        }
        
        std::cout << "DEBUG: Successfully found property: " << it->first 
                  << ", type: " << Symbols::Variables::TypeToString(it->second->getType()) 
                  << ", value: " << it->second->toString() << std::endl;
        
        // Return a reference to the actual property value, not a clone
        return it->second;
    }

    std::string toString() const override { return objectExpr_->toString() + "->" + propertyName_; }

  private:
    std::unique_ptr<ExpressionNode> objectExpr_;
    std::string                     propertyName_;
    std::string                     filename_;
    int                             line_;
    size_t                          column_;
};

}  // namespace Interpreter

#endif  // INTERPRETER_MEMBER_EXPRESSION_NODE_HPP
