#ifndef INTERPRETER_CLASS_DEFINITION_STATEMENT_NODE_HPP
#define INTERPRETER_CLASS_DEFINITION_STATEMENT_NODE_HPP

#include <string>
#include <vector>
#include <memory>

#include "Interpreter/StatementNode.hpp"
#include "Symbols/ClassRegistry.hpp"

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

    void interpret(Interpreter & /*interpreter*/) const override {
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
    }

    std::string toString() const override { return "ClassDefinition{ class=" + className_ + " }"; }
};

}  // namespace Interpreter

#endif  // INTERPRETER_CLASS_DEFINITION_STATEMENT_NODE_HPP
