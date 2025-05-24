#ifndef INTERPRETER_CLASS_DEFINITION_STATEMENT_NODE_HPP
#define INTERPRETER_CLASS_DEFINITION_STATEMENT_NODE_HPP

#include <algorithm>  // for std::reverse
#include <string>
#include <vector>

#include "Interpreter/Interpreter.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/StatementNode.hpp"

#include "Symbols/SymbolContainer.hpp"

namespace Interpreter {

/**
 * @brief AST node representing a class definition statement.
 */
class ClassDefinitionStatementNode : public StatementNode {
    std::string                                   className_;
    std::string                                   classNs_;
    std::vector<Symbols::SymbolContainer::PropertyInfo> privateProperties_;
    std::vector<Symbols::SymbolContainer::PropertyInfo> publicProperties_;
    std::vector<std::string>                      methodNames_;
    std::string                                   constructorName_;  // Added

  public:
    ClassDefinitionStatementNode(const std::string & className, const std::string & classNs,
                                 std::vector<Symbols::SymbolContainer::PropertyInfo> privateProps,
                                 std::vector<Symbols::SymbolContainer::PropertyInfo> publicProps,
                                 std::vector<std::string>                      methods,
                                 const std::string &                           constructorName,  // Added
                                 const std::string & filename, int line, size_t column) :
        StatementNode(filename, line, column),
        className_(className),
        classNs_(classNs),
        privateProperties_(std::move(privateProps)),
        publicProperties_(std::move(publicProps)),
        methodNames_(std::move(methods)),
        constructorName_(constructorName) {}  // Added

    void interpret(Interpreter & interpreter) const override {
        auto * sc = Symbols::SymbolContainer::instance();
        
        // Register the class itself
        sc->registerClass(className_);
        
        // Register private and public properties (privacy not enforced yet)
        for (const auto & prop : privateProperties_) {
            sc->addProperty(className_, prop.name, prop.type, true, prop.defaultValueExpr);
        }
        for (const auto & prop : publicProperties_) {
            sc->addProperty(className_, prop.name, prop.type, false, prop.defaultValueExpr);
        }
        
        // Register methods
        // Reverse the methodNames_ to fix the order issue
        std::vector<std::string> reversedMethods = methodNames_;
        std::reverse(reversedMethods.begin(), reversedMethods.end());
        
        for (const auto & method : reversedMethods) {
            sc->addMethod(className_, method);
        }
        
        // Register constructor if it exists
        if (!constructorName_.empty()) {
            // The constructor is already registered as a method above
            // We might want to set it as the constructor specifically in the future
        }
        
        // After registering methods in class registry, also register function symbols
        // for class methods by executing their declaration operations
        const std::string fileNs  = sc->currentScopeName();
        const std::string classNs = fileNs + Symbols::SymbolContainer::SCOPE_SEPARATOR + className_;
        
        // Execute all operations in the class namespace to register methods and their bodies
        auto ops = Operations::Container::instance()->getAll(classNs);
        
        for (const auto & op : ops) {
            interpreter.runOperation(*op);
        }
    }

    std::string toString() const override { return "ClassDefinition{ class=" + className_ + " }"; }
};

}  // namespace Interpreter

#endif  // INTERPRETER_CLASS_DEFINITION_STATEMENT_NODE_HPP
