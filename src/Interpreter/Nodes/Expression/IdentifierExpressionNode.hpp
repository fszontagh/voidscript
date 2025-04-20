#ifndef IDENTIFIER_EXPRESSION_NODE_HPP
#define IDENTIFIER_EXPRESSION_NODE_HPP

#include "Interpreter/ExpressionNode.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

class IdentifierExpressionNode : public ExpressionNode {
    std::string name_;

  public:
    explicit IdentifierExpressionNode(std::string name) : name_(std::move(name)) {}

    Symbols::Value evaluate(Interpreter & /*interpreter*/) const override {
        auto * sc = Symbols::SymbolContainer::instance();
        const std::string base_ns  = sc->currentScopeName();
        // Look in current scope's variables namespace
        const std::string var_ns   = base_ns + "::variables";
        if (sc->exists(name_, var_ns)) {
            return sc->get(var_ns, name_)->getValue();
        }
        const std::string const_ns = base_ns + "::constants";
        if (sc->exists(name_, const_ns)) {
            return sc->get(const_ns, name_)->getValue();
        }
        throw std::runtime_error("Identifier '" + name_ + "' not found in namespace: " + base_ns);
    }

    std::string toString() const override { return name_; }
};

}  // namespace Interpreter

#endif  // IDENTIFIER_EXPRESSION_NODE_HPP
