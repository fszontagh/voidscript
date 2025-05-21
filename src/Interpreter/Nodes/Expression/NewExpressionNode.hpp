#ifndef INTERPRETER_NEW_EXPRESSION_NODE_HPP
#define INTERPRETER_NEW_EXPRESSION_NODE_HPP

#include <iostream> // Required for std::cerr
#include <memory>
#include <string>
#include <vector>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Interpreter/ReturnException.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Parser/ParsedExpression.hpp"
#include "Symbols/FunctionSymbol.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"
#include "Symbols/Value.hpp"

// Forward declaration for expression builder
namespace Parser {
std::unique_ptr<Interpreter::ExpressionNode> buildExpressionFromParsed(const ParsedExpressionPtr & expr);
}

namespace Interpreter {

/**
 * @brief AST node for 'new' expressions, instantiating objects of a class.
 */
class NewExpressionNode : public ExpressionNode {
    std::string                                  className_;
    std::vector<std::unique_ptr<ExpressionNode>> args_;
    std::string                                  filename_;
    int                                          line_;
    size_t                                       column_;

  public:
    NewExpressionNode(const std::string & className, std::vector<std::unique_ptr<ExpressionNode>> && args,
                      const std::string & filename, int line, size_t column) :
        className_(className),
        args_(std::move(args)),
        filename_(filename),
        line_(line),
        column_(column) {}

    Symbols::ValuePtr evaluate(Interpreter & interpreter, std::string /*filename*/, int /*line*/,
                               size_t /*col */) const override {
        std::shared_ptr<Symbols::FunctionSymbol> construct_method_sym;  // Declare here
        auto &                                   registry = Modules::UnifiedModuleManager::instance();

        // Ensure class is defined
        if (!registry.hasClass(className_)) {
            throw Exception("Class not found: " + className_, filename_, line_, column_);
        }

        // Initialize object fields from class definition
        // const auto & class_definition_info = registry.getClassInfo(className_); // Renamed to avoid confusion
        // Using 'info' as it was originally, but it's the class definition from registry
        const auto &       info = registry.getClassInfo(className_);
        Symbols::ObjectMap obj;
        // Default initialization for all properties
        size_t             propCount = info.properties.size();
        for (size_t i = 0; i < propCount; ++i) {
            const auto & prop  = info.properties[i];
            auto         value = Symbols::ValuePtr::null(prop.type);
            if (prop.defaultValueExpr) {
                // Build and evaluate default expression
                auto exprNode = Parser::buildExpressionFromParsed(prop.defaultValueExpr);
                value         = exprNode->evaluate(interpreter);
                // std::cout << "expr. " << " prop.name: " << prop.name << " -> " << exprNode->toString()
                //           << " return type: " << Symbols::Variables::TypeToString(value.getType()) << "\n";
            }

            // Check type of default value if provided
            if (prop.defaultValueExpr && prop.type != value.getType() &&
                value.getType() != Symbols::Variables::Type::NULL_TYPE) {
                throw Exception("Invalid default property value type for property '" + prop.name +
                                    "'. Expected: " + Symbols::Variables::TypeToString(prop.type) +
                                    " got: " + Symbols::Variables::TypeToString(value.getType()) + " in class '" +
                                    className_ + "'",
                                filename_, line_, column_);
            }
            obj[prop.name] = value.clone();  // Clone default values to ensure instance owns its copy
        }

        // Embed class metadata for method dispatch
        obj["__class__"]              = className_;
        // Create the class instance ValuePtr
        Symbols::ValuePtr instance_vp = Symbols::ValuePtr::makeClassInstance(obj);

        // Attempt to find the 'construct' method symbol within the class's scope
        auto * sc = Symbols::SymbolContainer::instance();
        // Class methods are typically defined within the class's own scope.
        // The class scope name is simply className_ as per current SymbolContainer usage for classes.
        auto   class_scope_table =
            sc->getScopeTable(sc->currentScopeName() + Symbols::SymbolContainer::SCOPE_SEPARATOR + className_);
        if (class_scope_table) {
            // Functions are stored in a specific sub-scope (e.g., "functions") within the class scope table.
            Symbols::SymbolPtr found_symbol =
                class_scope_table->get(Symbols::SymbolContainer::DEFAULT_FUNCTIONS_SCOPE, "construct");
            if (found_symbol && found_symbol->getKind() == Symbols::Kind::Function) {
                construct_method_sym = std::static_pointer_cast<Symbols::FunctionSymbol>(found_symbol);
            }
        }

        if (construct_method_sym) {
            // Constructor found, proceed with argument evaluation and call
            std::vector<Symbols::ValuePtr> evaluated_constructor_args;
            evaluated_constructor_args.reserve(args_.size());
            for (const auto & arg_expr : args_) {
                evaluated_constructor_args.push_back(arg_expr->evaluate(interpreter).clone());
            }

            const auto & expected_params = construct_method_sym->parameters();
            if (evaluated_constructor_args.size() != expected_params.size()) {
                throw Exception("Argument count mismatch for constructor of class '" + className_ + "'. Expected " +
                                    std::to_string(expected_params.size()) + ", got " +
                                    std::to_string(evaluated_constructor_args.size()) + ".",
                                filename_, line_, column_);
            }

            // Optional: Parameter Type Checking
            for (size_t i = 0; i < expected_params.size(); ++i) {
                if (expected_params[i].type !=
                        Symbols::Variables::Type::UNDEFINED_TYPE &&  // Check if param type is specified
                    expected_params[i].type != evaluated_constructor_args[i].getType() &&
                    evaluated_constructor_args[i].getType() !=
                        Symbols::Variables::Type::NULL_TYPE) {  // Allow null to be passed
                    throw Exception("Type mismatch for constructor argument " + std::to_string(i + 1) + " ('" +
                                        expected_params[i].name + "') of class '" + className_ + "'. Expected " +
                                        Symbols::Variables::TypeToString(expected_params[i].type) + ", got " +
                                        Symbols::Variables::TypeToString(evaluated_constructor_args[i].getType()) + ".",
                                    filename_, line_, column_);
                }
            }

            Symbols::SymbolContainer * sc    = Symbols::SymbolContainer::instance();
            std::string constructor_scope_id = className_ + Symbols::SymbolContainer::SCOPE_SEPARATOR + "construct";
            std::string call_scope_name      = sc->enterFunctionCallScope(constructor_scope_id);

            // Bind "this"
            // The type of 'this' is the class itself, which is represented as an OBJECT internally for ValuePtr
            sc->add(Symbols::SymbolFactory::createVariable("this", instance_vp, call_scope_name,
                                                           Symbols::Variables::Type::CLASS));

            // Bind parameters
            for (size_t i = 0; i < expected_params.size(); ++i) {
                const auto &      param_info = expected_params[i];
                Symbols::ValuePtr arg_value  = evaluated_constructor_args[i];
                // If argument is null and param has a specific type, try to make the arg "null of that type"
                if (arg_value.getType() == Symbols::Variables::Type::NULL_TYPE &&
                    param_info.type != Symbols::Variables::Type::UNDEFINED_TYPE) {
                    arg_value = Symbols::ValuePtr::null(param_info.type);
                }
                sc->add(Symbols::SymbolFactory::createVariable(param_info.name, arg_value, call_scope_name,
                                                               arg_value.getType()));
            }

            // Execute constructor body
            auto ops = Operations::Container::instance()->getAll(constructor_scope_id);
            try {
                for (const auto & op : ops) {
                    interpreter.runOperation(*op);
                }
            } catch (const ReturnException & ret_ex) {
                // Constructor return values are typically ignored.
                // If a 'return;' or 'return value;' is encountered, it just exits the constructor.
            } catch (const Exception &) {              // Propagate interpreter exceptions
                sc->enterPreviousScope();              // Ensure scope is exited on error
                throw;
            } catch (const std::exception & std_ex) {  // Propagate standard exceptions
                sc->enterPreviousScope();              // Ensure scope is exited on error
                throw Exception(std_ex.what(), filename_, line_, column_);
            }

            sc->enterPreviousScope();

        } else {
            // No constructor found
            if (!args_.empty()) {
                throw Exception("Class '" + className_ +
                                    "' does not have a constructor (construct method), but arguments were provided.",
                                filename_, line_, column_);
            }
        }

        std::cerr << "[DEBUG NewExpressionNode] Returning instance_vp. State: " << instance_vp.getDebugStateString() << " for class: " << className_ << std::endl;
        return instance_vp;
    }

    std::string toString() const override {
        return "NewExpression{ class=" + className_ + ", args=" + std::to_string(args_.size()) + " }";
    }
};

}  // namespace Interpreter

#endif  // INTERPRETER_NEW_EXPRESSION_NODE_HPP
