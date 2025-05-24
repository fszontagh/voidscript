#include "Interpreter/Interpreter.hpp"

#include <iostream>

#include "Interpreter/ReturnException.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

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

Symbols::ValuePtr Interpreter::executeMethod(const Symbols::ValuePtr & objectValue, const std::string & methodName,
                                             const std::vector<Symbols::ValuePtr> & args) {
    if (!objectValue) {
        throw Exception("Cannot call method on null value", "-", 0, 0);
    }

    if (objectValue->getType() != Symbols::Variables::Type::OBJECT &&
        objectValue->getType() != Symbols::Variables::Type::CLASS) {
        throw Exception("Cannot call method on non-object value of type " +
                            Symbols::Variables::TypeToString(objectValue->getType()),
                        "-", 0, 0);
    }

    const auto & objMap = objectValue->get<Symbols::ObjectMap>();

    // Look for class name in the object
    std::string className;
    auto        classNameIt = objMap.find("$class_name");
    if (classNameIt == objMap.end()) {
        // Try alternative class name key
        classNameIt = objMap.find("__class__");
        if (classNameIt == objMap.end()) {
            throw Exception("Cannot call method on object without class name", "-", 0, 0);
        }
    }

    // Get the class name from the object
    const auto & classNameVal = classNameIt->second;
    if (classNameVal->getType() != Symbols::Variables::Type::STRING) {
        throw Exception("Invalid class name type", "-", 0, 0);
    }
    className = classNameVal->get<std::string>();

    // Save the previous "this" object
    auto previousThis = thisObject_;

    // Set the new "this" object
    thisObject_ = objectValue;

    try {
        // Get the method from the SymbolContainer
        auto & symbolContainer = Symbols::SymbolContainer::instance();

        if (!symbolContainer->hasClass(className)) {
            throw Exception("Class not found in SymbolContainer: " + className, "-", 0, 0);
        }

        std::string fullMethodName = className + Symbols::SymbolContainer::SCOPE_SEPARATOR + methodName;
        if (!symbolContainer->hasMethod(className, methodName)) {
            throw Exception(
                "Method '" + methodName + "' not found in class '" + className + "'", "-", 0,
                0);
        }

        // Execute the method
        auto result = symbolContainer->callMethod(className, methodName, args);
        //auto result = mgr.callFunction(fullMethodName, const_cast<std::vector<Symbols::ValuePtr> &>(args));

        // Restore the previous "this" object
        thisObject_ = previousThis;

        return result;
    } catch (const ReturnException & re) {
        // Special handling for return statements
        thisObject_ = previousThis;
        return re.value();
    } catch (const std::exception & e) {
        // Restore the previous "this" object even if an error occurred
        thisObject_ = previousThis;
        throw;
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
    if (debug_) {
        std::cerr << "[Debug][Interpreter] Operation: " << op.toString() << "\n";
    }

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

}  // namespace Interpreter
