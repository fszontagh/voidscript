#include "Parser/ParsedExpression.hpp"
#include "Modules/UnifiedModuleManager.hpp"

namespace Parser {

Symbols::Variables::Type ParsedExpression::getType() const {
    switch (kind) {
        case Kind::Literal:
            return value.getType();
            break;

        case Kind::Variable:
            {
                // Use findSymbol which correctly searches variables and constants up the scope chain.
                auto symbol = Symbols::SymbolContainer::instance()->findSymbol(name);
                if (!symbol) {
                    // findSymbol searches current scope and up for variables/constants
                    throw std::runtime_error("Unknown variable or constant: " + name +
                                             " (searched from scope: " + Symbols::SymbolContainer::instance()->currentScopeName() + ")" +
                                             " File: " + filename + ":" + std::to_string(line));
                }
                // findSymbol returns a SymbolPtr, which could be VariableSymbol or ConstantSymbol.
                // Both have getValue().
                return symbol->getValue().getType();
            }

        case Kind::Binary:
            {
                auto lhsType = lhs->value.getType();
                //auto rhsType = rhs->value.getType();
                return lhsType;  // In binary expressions, operand types match, so we can return the left-hand type
            }

        case Kind::Unary:
            {
                //auto operandType = op.
                if (op == "!") {
                    return Symbols::Variables::Type::BOOLEAN;  // Because the '!' operator expects a boolean type
                }
                break;
            }
        case Kind::Call:
            {
                // First check UnifiedModuleManager for built-in functions
                auto& mgr = Modules::UnifiedModuleManager::instance();
                if (mgr.hasFunction(name)) {
                    return mgr.getFunctionReturnType(name);
                }

                // If not found in UnifiedModuleManager, check the current scope for user-defined functions
                auto sc = Symbols::SymbolContainer::instance();
                auto current_scope_table = sc->getScopeTable(sc->currentScopeName());
                Symbols::SymbolPtr symbol = nullptr;
                if (current_scope_table) {
                    symbol = current_scope_table->get(Symbols::SymbolContainer::DEFAULT_FUNCTIONS_SCOPE, name);
                }

                if (!symbol) {
                     throw std::runtime_error("Unknown function: " + name + " in current scope: " + sc->currentScopeName() + 
                                             " File: " + filename + ":" + std::to_string(line));
                }
                // FunctionSymbol holds return type
                auto funcSym = std::dynamic_pointer_cast<Symbols::FunctionSymbol>(symbol);
                if (!funcSym) {
                    // Should not happen if it was found in DEFAULT_FUNCTIONS_SCOPE and is a function
                    throw std::runtime_error("Symbol " + name + " found but is not a function." +
                                             " File: " + filename + ":" + std::to_string(line));
                }
                return funcSym->returnType();
            }
        case Kind::Object:
            return Symbols::Variables::Type::OBJECT;

        default:
            throw std::runtime_error("Unknown expression kind");
    }

    throw std::runtime_error("Could not determine type for expression");
}

std::string ParsedExpression::toString() const {
    switch (kind) {
        case Kind::Literal:
            return Symbols::Value::to_string(value);
        case Kind::Variable:
            return name;
        case Kind::Binary:
            return "(" + lhs->toString() + " " + op + " " + rhs->toString() + ")";
        case Kind::Unary:
            return "(" + op + rhs->toString() + ")";
        case Kind::Call:
            {
                std::string result = name + "(";
                for (size_t i = 0; i < args.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += args[i]->toString();
                }
                result += ")";
                return result;
            }
        case Kind::MethodCall:
            {
                std::string result = lhs->toString() + "->" + name + "(";
                for (size_t i = 0; i < args.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += args[i]->toString();
                }
                result += ")";
                return result;
            }
        case Kind::Object:
            {
                std::string result = "{";
                for (size_t i = 0; i < objectMembers.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += objectMembers[i].first + ": " + objectMembers[i].second->toString();
                }
                result += "}";
                return result;
            }
        case Kind::New:
            {
                std::string result = "new " + name + "(";
                for (size_t i = 0; i < args.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += args[i]->toString();
                }
                result += ")";
                return result;
            }
        case Kind::Member:
            return objectMembers[0].second->toString() + "->" + objectMembers[0].first;
        default:
            return "Unknown expression kind";
    }
}

}  // namespace Parser 