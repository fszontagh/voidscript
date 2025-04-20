#ifndef INTERPRETER_CLASS_DEFINITION_STATEMENT_NODE_HPP
#define INTERPRETER_CLASS_DEFINITION_STATEMENT_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include "Interpreter/Interpreter.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/StatementNode.hpp"
#include "Symbols/ClassRegistry.hpp"
#include "Symbols/SymbolContainer.hpp"

namespace Interpreter {

/**
 * @brief AST node representing a class definition statement.
 */
class ClassDefinitionStatementNode : public StatementNode {
    std::string                                   className_;
    std::vector<Symbols::ClassInfo::PropertyInfo> privateProperties_;
    std::vector<Symbols::ClassInfo::PropertyInfo> publicProperties_;
    std::vector<std::string>                      methodNames_;

  public:
    ClassDefinitionStatementNode(const std::string &                           className,
                                 std::vector<Symbols::ClassInfo::PropertyInfo> privateProps,
                                 std::vector<Symbols::ClassInfo::PropertyInfo> publicProps,
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
        auto & registry = Symbols::ClassRegistry::instance();
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
        for (const auto & method : methodNames_) {
            registry.addMethod(className_, method);
        }
        // After registering methods in class registry, also register function symbols
        // for class methods by executing their declaration operations
        // The method declaration operations were recorded under namespace: fileNs.className
        const std::string fileNs  = sc->currentScopeName();
        const std::string classNs = fileNs + "::" + className_;
        auto              ops     = Operations::Container::instance()->getAll(classNs);
        for (const auto & op : ops) {
            if (op->type == Operations::Type::FuncDeclaration) {
                interpreter.runOperation(*op);
            }
        }
    }

    std::string toString() const override { return "ClassDefinition{ class=" + className_ + " }"; }
};

}  // namespace Interpreter

#endif  // INTERPRETER_CLASS_DEFINITION_STATEMENT_NODE_HPP
