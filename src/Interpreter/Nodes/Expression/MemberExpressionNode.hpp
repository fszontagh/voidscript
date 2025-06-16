// filepath: /home/fszontagh/soniscript/src/Interpreter/Nodes/Expression/MemberExpressionNode.hpp
#ifndef INTERPRETER_MEMBER_EXPRESSION_NODE_HPP
#define INTERPRETER_MEMBER_EXPRESSION_NODE_HPP

#include <memory>
#include <string>
// #include <sstream> // Not strictly needed if only using toString()

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Symbols/SymbolContainer.hpp"
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

        auto objVal = objectExpr_->evaluate(interpreter, filename_, line_, column_);


        // Allow member access on plain objects and class instances
        if (objVal->getType() != Symbols::Variables::Type::OBJECT &&
            objVal->getType() != Symbols::Variables::Type::CLASS) {
            throw Exception("Attempted to access member '" + propertyName_ + "' of non-object", filename_, line_,
                            column_);
        }

        const auto & map = objVal->get<Symbols::ObjectMap>();
        std::string keyToLookup = propertyName_; // Default to the exact name

        if (objVal->getType() == Symbols::Variables::Type::CLASS) {
            auto classMetaIt = map.find("$class_name");
            if (classMetaIt != map.end() && classMetaIt->second->getType() == Symbols::Variables::Type::STRING) {
                std::string className = classMetaIt->second->get<std::string>();
                auto* sc = Symbols::SymbolContainer::instance();

                if (sc->hasClass(className)) {
                    // 1. Check for method first
                    if (sc->hasMethod(className, propertyName_)) {
                        // Check access control for methods - block private method access from outside
                        if (!interpreter.canAccessPrivateMember(className, propertyName_, false)) {
                            throw Exception("Cannot access private method '" + propertyName_ + "' from outside class '" + className + "'", filename_, line_, column_);
                        }
                        return Symbols::ValuePtr(propertyName_); // Return method name string
                    }

                    // 2. Check for properties - ALWAYS check access control before any map lookup
                    std::string actualPropertyName;
                    bool propertyFound = false;
                    
                    // Check if property is registered (either direct name or $-prefixed)
                    if (sc->hasProperty(className, propertyName_)) {
                        actualPropertyName = propertyName_;
                        propertyFound = true;
                    } else if (propertyName_[0] != '$' && sc->hasProperty(className, "$" + propertyName_)) {
                        actualPropertyName = "$" + propertyName_;
                        propertyFound = true;
                    }
                    
                    if (propertyFound) {
                        // Property is registered, check access control FIRST
                        if (!interpreter.canAccessPrivateMember(className, actualPropertyName, true)) {
                            throw Exception("Cannot access private property '" + propertyName_ + "' from outside class '" + className + "'", filename_, line_, column_);
                        }
                        
                        // Access is allowed, determine the key to lookup in the map
                        if (map.find(propertyName_) != map.end()) {
                            keyToLookup = propertyName_;
                        } else if (propertyName_[0] != '$' && map.find("$" + propertyName_) != map.end()) {
                            keyToLookup = "$" + propertyName_;
                        } else {
                            // Property is registered and access is allowed, but not found in this instance
                            throw Exception("Property '" + propertyName_ + "' is defined by class '" + className + "' but not initialized in this instance", filename_, line_, column_);
                        }
                    } else {
                        // Property not registered in class
                        throw Exception("Property '" + propertyName_ + "' is not defined in class '" + className + "'", filename_, line_, column_);
                    }
                }
            }
        }

        // Final lookup in map using the determined keyToLookup
        auto it = map.find(keyToLookup);
        
        if (it == map.end()) {
            // If it's a class and we haven't found it, but it was registered (e.g. $prop that wasn't initialized)
            if (objVal->getType() == Symbols::Variables::Type::CLASS) {
                 auto classMetaIt = map.find("$class_name");
                 if (classMetaIt != map.end() && classMetaIt->second->getType() == Symbols::Variables::Type::STRING) {
                    std::string className = classMetaIt->second->get<std::string>();
                    auto* sc = Symbols::SymbolContainer::instance();
                    if (sc->hasClass(className)) {
                        // Check if the property is registered in the class (either direct name or $-prefixed)
                        if (sc->hasProperty(className, propertyName_)) {
                            // Check access control first before saying it's not initialized
                            if (!interpreter.canAccessPrivateMember(className, propertyName_, true)) {
                                throw Exception("Cannot access private property '" + propertyName_ + "' from outside class '" + className + "'", filename_, line_, column_);
                            }
                            throw Exception("Property '" + propertyName_ + "' is defined by class '" + className + "' but not initialized or found in this instance.", filename_, line_, column_);
                        } else if (propertyName_[0] != '$' && sc->hasProperty(className, "$" + propertyName_)) {
                            // Check access control for $-prefixed property
                            if (!interpreter.canAccessPrivateMember(className, "$" + propertyName_, true)) {
                                throw Exception("Cannot access private property '" + propertyName_ + "' from outside class '" + className + "'", filename_, line_, column_);
                            }
                            throw Exception("Property '" + propertyName_ + "' is defined by class '" + className + "' but not initialized or found in this instance.", filename_, line_, column_);
                        }
                    }
                 }
            }
            throw Exception("Property '" + keyToLookup + "' (resolved from '" + propertyName_ + "') not found in object", filename_, line_, column_);
        }
        
        // Check for null property value
        if (!it->second) {
            throw Exception("Property '" + keyToLookup + "' points to a null value pointer", filename_, line_, column_);
        }
        
        if (it->second->isNULL()) {
            throw Exception("Property '" + keyToLookup + "' is null in MemberExpressionNode", filename_, line_, column_);
        }
        
        // Return a reference to the actual property value, not a clone
        if (!it->second) {
            throw Exception("CRITICAL: Property value is NULL pointer - this should never happen", filename_, line_, column_);
        }
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
