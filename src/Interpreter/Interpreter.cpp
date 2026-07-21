#include "Interpreter/Interpreter.hpp"

#include <iostream>

#include "Interpreter/ReturnException.hpp"
#include "Interpreter/ThrowException.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"
#include "Symbols/FunctionSymbol.hpp"

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
        // Presence check, NOT truthiness: `if (thisObject_)` would have asked whether
        // the object is "true", so a class instance with an empty member map would have
        // read as absent and silently skipped this access check.
        if (!thisObject_->is_null() && thisObject_->getType() == Symbols::Variables::Type::CLASS) {
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
    // Publish this interpreter as the thread's current one for the duration of the run,
    // so native modules can call back into script functions (callUserFunction).
    Interpreter * prev = current_;
    current_           = this;
    struct Restore {
        Interpreter *& slot;
        Interpreter *  saved;
        ~Restore() { slot = saved; }
    } restore{ current_, prev };

    // Determine namespace to execute
    const std::string ns = Symbols::SymbolContainer::instance()->currentScopeName();
    for (const auto & operation : Operations::Container::instance()->getAll(ns)) {
        runOperation(*operation);
    }
}

Symbols::ValuePtr Interpreter::callUserFunction(const std::string & name,
                                                const std::vector<Symbols::ValuePtr> & args) {
    if (!current_) {
        throw Exception("callUserFunction: no interpreter is running on this thread", "-", 0, 0);
    }
    Interpreter &            interpreter = *current_;
    Symbols::SymbolContainer * sc        = Symbols::SymbolContainer::instance();

    // Resolve the function symbol, walking up the scope hierarchy like a normal call.
    std::string                              lookupNs = sc->currentScopeName();
    std::shared_ptr<Symbols::FunctionSymbol> funcSym;
    while (true) {
        if (auto scope_table = sc->getScopeTable(lookupNs)) {
            auto sym = scope_table->get(Symbols::SymbolContainer::DEFAULT_FUNCTIONS_SCOPE, name);
            if (sym && sym->getKind() == Symbols::Kind::Function) {
                funcSym = std::static_pointer_cast<Symbols::FunctionSymbol>(sym);
                break;
            }
        }
        auto pos = lookupNs.rfind(Symbols::SymbolContainer::SCOPE_SEPARATOR);
        if (pos == std::string::npos) {
            break;
        }
        lookupNs = lookupNs.substr(0, pos);
        if (lookupNs.empty()) {
            break;
        }
    }
    if (!funcSym) {
        throw Exception("callUserFunction: function not found: " + name, "-", 0, 0);
    }

    const auto & params = funcSym->parameters();
    if (params.size() != args.size()) {
        throw Exception("callUserFunction: '" + name + "' expects " + std::to_string(params.size()) +
                            " args, got " + std::to_string(args.size()),
                        "-", 0, 0);
    }

    const std::string canonical = funcSym->context().empty()
                                      ? name
                                      : funcSym->context() + Symbols::SymbolContainer::SCOPE_SEPARATOR + name;
    const std::string callScope = canonical + Symbols::SymbolContainer::CALL_SCOPE + std::to_string(get_unique_call_id());

    sc->create(callScope);
    for (size_t i = 0; i < params.size(); ++i) {
        sc->addVariable(Symbols::SymbolFactory::createVariable(params[i].name, args[i].clone(), callScope));
    }

    Symbols::ValuePtr returnValue;
    for (const auto & op : Operations::Container::instance()->getAll(canonical)) {
        try {
            interpreter.runOperation(*op);
        } catch (const ReturnException & ret) {
            returnValue = ret.value();
            break;
        }
    }

    if (sc->currentScopeName() == callScope) {
        sc->enterPreviousScope();
    } else {
        sc->validateAndCleanupScopeStack(callScope);
    }
    return returnValue;
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
            case Operations::Type::ControlFlow:  // switch
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
        }
        // No `default:` arm on purpose. This switch covers every Operations::Type, and
        // leaving it exhaustive means -Werror=switch catches a newly added type at
        // compile time. A default arm previously swallowed a missing
        // Operations::Type::ControlFlow case and made every `switch` statement in the
        // language fail at runtime with "Unknown operation type".
    } catch (const BaseException &) {
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
    
    bool hasMethod = sc->hasMethod(className, methodName);
    
    if (!hasMethod) {
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
