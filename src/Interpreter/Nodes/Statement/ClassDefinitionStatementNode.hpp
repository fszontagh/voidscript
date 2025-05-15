#ifndef INTERPRETER_CLASS_DEFINITION_STATEMENT_NODE_HPP
#define INTERPRETER_CLASS_DEFINITION_STATEMENT_NODE_HPP

#include <algorithm>  // for std::reverse
#include <string>
#include <vector>

#include "Interpreter/Interpreter.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/SymbolContainer.hpp"

namespace Interpreter {

/**
 * @brief AST node representing a class definition statement.
 */
class ClassDefinitionStatementNode : public StatementNode {
    std::string                                   className_;
    std::vector<Modules::ClassInfo::PropertyInfo> privateProperties_;
    std::vector<Modules::ClassInfo::PropertyInfo> publicProperties_;
    std::vector<std::string>                      methodNames_;

  public:
    ClassDefinitionStatementNode(const std::string &                           className,
                                 std::vector<Modules::ClassInfo::PropertyInfo> privateProps,
                                 std::vector<Modules::ClassInfo::PropertyInfo> publicProps,
                                 std::vector<std::string> methods, const std::string & filename, int line,
                                 size_t column) :
        StatementNode(filename, line, column),
        className_(className),
        privateProperties_(std::move(privateProps)),
        publicProperties_(std::move(publicProps)),
        methodNames_(std::move(methods)) {}

    void interpret(Interpreter & interpreter) const override {
        auto * sc       = Symbols::SymbolContainer::instance();
        // Register class and its members in class registry
        auto & registry = Modules::UnifiedModuleManager::instance();
        // Register the class itself
        registry.registerClass(className_);
        // Register private and public properties (privacy not enforced yet)
        for (const auto & prop : privateProperties_) {
            registry.addProperty(className_, prop.name, prop.type, prop.defaultValueExpr);
        }
        for (const auto & prop : publicProperties_) {
            registry.addProperty(className_, prop.name, prop.type, prop.defaultValueExpr);
        }
        // Register methods
        // Reverse the methodNames_ to fix the order issue
        std::vector<std::string> reversedMethods = methodNames_;
        std::reverse(reversedMethods.begin(), reversedMethods.end());
        for (const auto & method : reversedMethods) {
            registry.addMethod(className_, method);
        }
        // After registering methods in class registry, also register function symbols
        // for class methods by executing their declaration operations
        const std::string fileNs  = sc->currentScopeName();
        const std::string classNs = fileNs + Symbols::SymbolContainer::SCOPE_SEPARATOR + className_;
        // Execute all operations in the class namespace to register methods and their bodies
        auto              ops     = Operations::Container::instance()->getAll(classNs);
        for (const auto & op : ops) {
            interpreter.runOperation(*op);
        }
    }

    std::string toString() const override { return "ClassDefinition{ class=" + className_ + " }"; }
};

}  // namespace Interpreter

#endif  // INTERPRETER_CLASS_DEFINITION_STATEMENT_NODE_HPP
