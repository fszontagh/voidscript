#include "Interpreter/Interpreter.hpp"

#include <iostream>

#include "Interpreter/ReturnException.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/EnumSymbol.hpp" // Added for EnumSymbol
#include "Nodes/Statement/EnumDeclarationNode.hpp" // Added for EnumDeclarationNode
#include "Nodes/Statement/SwitchStatementNode.hpp" // Added for SwitchStatementNode
#include "Nodes/Statement/BreakNode.hpp"         // Added for BreakNode
#include "Interpreter/BreakException.hpp"        // Added for BreakException
#include "Interpreter/ExpressionNode.hpp"        // For ExpressionNode::evaluate


namespace Interpreter {

void Interpreter::setThisObject(const Symbols::ValuePtr & obj) {
    thisObject_ = obj;
}

void Interpreter::clearThisObject() {
    thisObject_ = Symbols::ValuePtr();
}

const Symbols::ValuePtr & Interpreter::getThisObject() const {
    return thisObject_;
}

void Interpreter::setCurrentClass(const std::string& className) {
    currentClassName_ = className;
}

void Interpreter::clearCurrentClass() {
    currentClassName_.clear();
}

const std::string& Interpreter::getCurrentClass() const {
    return currentClassName_;
}

bool Interpreter::canAccessPrivateMember(const std::string& targetClassName,
                                        const std::string& memberName,
                                        bool isProperty) const {
    auto* sc = Symbols::SymbolContainer::instance();
    
    try {
        // Check if the class exists first
        if (!sc->hasClass(targetClassName)) {
            // If class doesn't exist, treat as public access (shouldn't happen in normal cases)
            return true;
        }
        
        // Check if the member is private
        bool isPrivate = isProperty ?
            sc->isPropertyPrivate(targetClassName, memberName) :
            sc->isMethodPrivate(targetClassName, memberName);
        
        // If it's not private, access is always allowed
        if (!isPrivate) {
            return true;
        }
        
        // If it's private, access is only allowed if we're currently executing within the same class
        // or if we're accessing via $this within the class
        if (!currentClassName_.empty() && currentClassName_ == targetClassName) {
            return true;
        }
        
        // Also check if we have a thisObject and it belongs to the target class
        if (thisObject_ && thisObject_->getType() == Symbols::Variables::Type::CLASS) {
            const auto& objMap = thisObject_->get<Symbols::ObjectMap>();
            auto classMetaIt = objMap.find("$class_name");
            if (classMetaIt != objMap.end() &&
                classMetaIt->second->getType() == Symbols::Variables::Type::STRING) {
                std::string thisClassName = classMetaIt->second->get<std::string>();
                if (thisClassName == targetClassName) {
                    return true;
                }
            }
        }
        
        return false;
        
    } catch (const std::exception& e) {
        // If there's any exception during access checking, default to denying access
        // This prevents crashes and ensures security by defaulting to restrictive access
        return false;
    }
}

void Interpreter::run() {
    // Determine namespace to execute
    const std::string ns = Symbols::SymbolContainer::instance()->currentScopeName();
    for (const auto & operation : Operations::Container::instance()->getAll(ns)) {
        runOperation(*operation);
    }
}

void Interpreter::runOperation(const Operations::Operation & op) {

    if (!op.statement && op.type != Operations::Type::Error) {
        throw Exception("Invalid operation: missing statement", "-", 0, 0);
    }

    try {
        switch (op.type) {
            // Basic statements
            case Operations::Type::Declaration:
            case Operations::Type::Assignment:
            case Operations::Type::Expression:
                op.statement->interpret(*this);
                break;

            // Function-related operations
            case Operations::Type::FuncDeclaration:
            case Operations::Type::MethodDeclaration:
            case Operations::Type::FunctionCall:
            case Operations::Type::MethodCall:
            case Operations::Type::Return:
                op.statement->interpret(*this);
                break;

            // Control flow
            case Operations::Type::Conditional:
            case Operations::Type::Loop:
            case Operations::Type::While:
                op.statement->interpret(*this);
                break;

            // Flow control statements
            case Operations::Type::Break:
            case Operations::Type::Continue:
                throw Exception("Break/Continue not implemented", "-", 0, 0);

            // Module system
            case Operations::Type::Import:
                throw Exception("Import not implemented", "-", 0, 0);

            // Special cases
            case Operations::Type::Block:
                op.statement->interpret(*this);
                break;

            case Operations::Type::Error:
                throw Exception("Error operation encountered", "-", 0, 0);

            default:
                throw Exception("Unknown operation type", "-", 0, 0);
        }
    } catch (const Exception &) {
        throw;
    } catch (const std::exception & e) {
        throw Exception(e.what(), "-", 0, 0);
    }
}

unsigned long long Interpreter::get_unique_call_id() {
    return next_call_id_++;
}

Symbols::ValuePtr Interpreter::executeMethod(const Symbols::ValuePtr& objectValue,
                                           const std::string& methodName,
                                           const std::vector<Symbols::ValuePtr>& args) {
    // Get the class name from the object
    if (objectValue->getType() != Symbols::Variables::Type::CLASS) {
        throw Exception("Cannot execute method on non-class object", "-", 0, 0);
    }
    
    const auto& objMap = objectValue->get<Symbols::ObjectMap>();
    auto classMetaIt = objMap.find("$class_name");
    if (classMetaIt == objMap.end() || classMetaIt->second->getType() != Symbols::Variables::Type::STRING) {
        throw Exception("Object missing class metadata", "-", 0, 0);
    }
    
    std::string className = classMetaIt->second->get<std::string>();
    auto* sc = Symbols::SymbolContainer::instance();
    
    if (!sc->hasMethod(className, methodName)) {
        throw Exception("Method '" + methodName + "' not found in class '" + className + "'", "-", 0, 0);
    }
    
    // Check access control
    if (!canAccessPrivateMember(className, methodName, false)) {
        throw Exception("Cannot access private method '" + methodName + "' of class '" + className + "'", "-", 0, 0);
    }
    
    // Set the execution context
    std::string previousClass = currentClassName_;
    Symbols::ValuePtr previousThis = thisObject_;
    
    setCurrentClass(className);
    setThisObject(objectValue);
    
    try {
        // Execute the method using the symbol container's method call mechanism
        const std::vector<Symbols::ValuePtr>& constArgs = args;
        Symbols::ValuePtr result = sc->callMethod(className, methodName, constArgs);
        
        // Restore previous context
        currentClassName_ = previousClass;
        thisObject_ = previousThis;
        
        return result;
    } catch (...) {
        // Restore previous context on exception
        currentClassName_ = previousClass;
        thisObject_ = previousThis;
        throw;
    }
}

// Visitor method implementations
// Removed Visit methods for EnumDeclarationNode, SwitchStatementNode, and BreakNode as per refactoring.

}  // namespace Interpreter
