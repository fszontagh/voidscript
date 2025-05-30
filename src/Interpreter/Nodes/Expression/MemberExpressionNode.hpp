// filepath: /home/fszontagh/soniscript/src/Interpreter/Nodes/Expression/MemberExpressionNode.hpp
#ifndef INTERPRETER_MEMBER_EXPRESSION_NODE_HPP
#define INTERPRETER_MEMBER_EXPRESSION_NODE_HPP

#include <iostream> // Required for std::cerr, std::endl
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
        // +++ Add New Logging +++
        std::cerr << "[DEBUG MEMBER_EXPR] Evaluating member access: " << objectExpr_->toString() << "->" << propertyName_
                  << " (File: " << filename_ << ":" << line_ << ")" << std::endl;
        // +++ End New Logging +++

        auto objVal = objectExpr_->evaluate(interpreter, filename_, line_, column_);

        // +++ Add New Logging +++
        std::cerr << "[DEBUG MEMBER_EXPR]   LHS object evaluated to: " << objVal->toString();
        if (objVal->getType() == Symbols::Variables::Type::CLASS || objVal->getType() == Symbols::Variables::Type::OBJECT) {
            std::cerr << " Properties: " << std::endl;
            for(const auto& pair : objVal->get<Symbols::ObjectMap>()){
                std::cerr << "[DEBUG MEMBER_EXPR]     LHS Property: " << pair.first << " = " << pair.second->toString() << std::endl;
            }
        } else {
            std::cerr << std::endl;
        }
        // +++ End New Logging +++

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
                        std::cerr << "[DEBUG MEMBER_EXPR]   Property '" << propertyName_ << "' identified as a METHOD of class '" << className << "'" << std::endl;
                        return Symbols::ValuePtr(propertyName_); // Return method name string
                    }

                    // 2. Determine key for map lookup: prefer direct name if in map, else try $-prefixed if registered and in map.
                    auto direct_it = map.find(propertyName_);
                    if (direct_it != map.end()) {
                        keyToLookup = propertyName_;
                        std::cerr << "[DEBUG MEMBER_EXPR]   Using direct key '" << keyToLookup << "' (found in instance map)." << std::endl;
                    } else if (propertyName_[0] != '$') {
                        std::string prefixedName = "$" + propertyName_;
                        auto prefixed_it = map.find(prefixedName);
                        if (prefixed_it != map.end()) {
                            // Ensure this $-prefixed name is actually a registered property for the class
                            if (sc->hasProperty(className, prefixedName)) {
                                keyToLookup = prefixedName;
                                std::cerr << "[DEBUG MEMBER_EXPR]   Using prefixed key '" << keyToLookup << "' (original not in map, prefixed is in map and registered)." << std::endl;
                            } else {
                                 std::cerr << "[DEBUG MEMBER_EXPR]   Original key '" << propertyName_ << "' not in map. Prefixed key '" << prefixedName << "' in map but not registered. Using original for final attempt." << std::endl;
                                // keyToLookup remains propertyName_
                            }
                        } else {
                             std::cerr << "[DEBUG MEMBER_EXPR]   Neither direct key '" << propertyName_ << "' nor prefixed key '" << prefixedName << "' found in instance map." << std::endl;
                             // keyToLookup remains propertyName_, check if it's a registered property for a better error
                             if (!sc->hasProperty(className, propertyName_) && !sc->hasProperty(className, prefixedName)) {
                                throw Exception("Neither method nor property '" + propertyName_ + "' (or '$" + propertyName_ + "') is defined in class '" + className + "'", filename_, line_, column_);
                             }
                        }
                    } else { // propertyName_ already starts with '$'
                        // If $propertyName is not in map, check if it's a registered property for error message
                        if (!sc->hasProperty(className, keyToLookup)) { // keyToLookup is propertyName_ here
                             throw Exception("Property '" + keyToLookup + "' is not defined in class '" + className + "'", filename_, line_, column_);
                        }
                         std::cerr << "[DEBUG MEMBER_EXPR]   Key '" << keyToLookup << "' (starts with $) not found in instance map, but is a registered property." << std::endl;
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
                    if (sc->hasClass(className) && (sc->hasProperty(className, propertyName_) || (propertyName_[0] != '$' && sc->hasProperty(className, "$" + propertyName_)))) {
                         throw Exception("Property '" + propertyName_ + "' is defined by class '" + className + "' but not initialized or found in this instance.", filename_, line_, column_);
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
        // +++ Add New Logging (already existed, but context of keyToLookup is important) +++
        std::cerr << "[DEBUG MEMBER_EXPR]   Accessing property using key: '" << keyToLookup
                  << "' (original: '" << propertyName_ << "')" << std::endl;
        if (it != map.end()) { // This check is redundant due to the throw above, but harmless
            std::cerr << "[DEBUG MEMBER_EXPR]   Property '" << keyToLookup << "' found. Value: " << it->second->toString() << std::endl;
        }
        // +++ End New Logging +++
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
