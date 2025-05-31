#ifndef IDENTIFIER_EXPRESSION_NODE_HPP
#define IDENTIFIER_EXPRESSION_NODE_HPP

#include <iostream>  // Required for std::cerr
#include <string>
#include <vector> // For string splitting

#include "Interpreter/ExpressionNode.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/EnumSymbol.hpp" // For EnumSymbol
#include "Interpreter/Interpreter.hpp" // For Interpreter::Exception

namespace Interpreter {

class IdentifierExpressionNode : public ExpressionNode {
    std::string name_;

  public:
    explicit IdentifierExpressionNode(std::string name) : name_(std::move(name)) {
        // The base ExpressionNode members (filename, line, column) should be set by the parser
        // when this node is created.
    }

    Symbols::ValuePtr evaluate(Interpreter & interpreter_instance, std::string filename_param, int line_param,
                               size_t column_param) const override {
        // Use node's own location info if available, otherwise params.
        // Assuming parser correctly sets these on ExpressionNode.
        const std::string& eval_filename = this->filename.empty() ? filename_param : this->filename;
        int eval_line = this->line == 0 ? line_param : this->line;
        size_t eval_column = this->column == 0 ? column_param : this->column;

        auto * sc = Symbols::SymbolContainer::instance();

        // +++ Add New Logging +++
        std::cerr << "[DEBUG IDENTIFIER] Evaluating identifier: '" << name_ << "'"
                  << " (File: " << eval_filename << ":" << eval_line << ")" << std::endl;
        if (interpreter_instance.getThisObject() && !interpreter_instance.getThisObject()->isNULL()) {
            std::cerr << "[DEBUG IDENTIFIER]   Interpreter 'thisObject' is SET to: "
                      << interpreter_instance.getThisObject()->toString() << std::endl;
        } else {
            std::cerr << "[DEBUG IDENTIFIER]   Interpreter 'thisObject' is NOT SET or is NULL." << std::endl;
        }
        // +++ End New Logging +++

        // Check for scope resolution operator "::"
        size_t scope_res_pos = name_.find("::");
        if (scope_res_pos != std::string::npos) {
            std::string scope_name = name_.substr(0, scope_res_pos);
            std::string member_name = name_.substr(scope_res_pos + 2);

            if (scope_name.empty() || member_name.empty()) {
                throw Exception("Invalid scope resolution format: '" + name_ + "'", eval_filename, eval_line, eval_column);
            }

            // Try to find the scope name as a symbol (should be an EnumSymbol)
            // We need to search in the current and parent scopes, or globally depending on language rules for enums.
            // Assuming SymbolContainer::get searches appropriately.
            auto enum_symbol_ptr = sc->get("", scope_name);

            if (!enum_symbol_ptr) {
                throw Exception("Enum '" + scope_name + "' not found.", eval_filename, eval_line, eval_column);
            }

            if (enum_symbol_ptr->kind() != Symbols::Kind::ENUM) {
                throw Exception("Symbol '" + scope_name + "' is not an enum.", eval_filename, eval_line, eval_column);
            }

            // Cast to EnumSymbol to access its members
            auto actual_enum_symbol = std::dynamic_pointer_cast<Symbols::EnumSymbol>(enum_symbol_ptr);
            if (!actual_enum_symbol) {
                 // This should not happen if kind() == ENUM, but as a safeguard
                throw Exception("Internal error: Symbol '" + scope_name + "' identified as ENUM but failed to cast.", eval_filename, eval_line, eval_column);
            }

            std::optional<int> member_value = actual_enum_symbol->GetValue(member_name);
            if (!member_value) {
                throw Exception("Member '" + member_name + "' not found in enum '" + scope_name + "'.", eval_filename, eval_line, eval_column);
            }
            return Symbols::ValuePtr(member_value.value()); // Enum members are integers
        }

        // Original logic for non-scoped identifiers
        if (name_ == "NULL" || name_ == "null") {
            return Symbols::ValuePtr::null(Symbols::Variables::Type::NULL_TYPE);
        }
        
        if (name_ == "this") {
            // 'this' logic might need to use interpreter_instance if 'this' is managed by Interpreter state
            auto thisSymbol = sc->getVariable("this"); // Or interpreter_instance.getThisObject();
            if (thisSymbol) { // Assuming getVariable returns a symbol that has a value
                return thisSymbol->getValue();
            }
            // Fallback to interpreter's 'this' object if direct symbol not found (e.g. in method calls)
            const auto& interpreterThis = interpreter_instance.getThisObject();
            if (interpreterThis && !interpreterThis->isNULL()) { // Check if interpreter's 'this' is valid
                 return interpreterThis;
            }
            throw Exception("Keyword 'this' not found or not valid in current context.", eval_filename, eval_line, eval_column);
        }

        // Logic for other identifiers
        Symbols::SymbolPtr symbol_from_scope; // Use SymbolPtr
        symbol_from_scope = sc->getVariable(name_);
        if (symbol_from_scope) {
            // +++ Add New Logging +++
            std::cerr << "[DEBUG IDENTIFIER]   Found '" << name_ << "' as VARIABLE in scope. Accessing its value..." << std::endl;
            const auto& val_ref = symbol_from_scope->getValue(); // Triggers Symbol::getValue() log
            std::cerr << "[DEBUG IDENTIFIER]   ValuePtr for VARIABLE '" << name_ << "' obtained from getValue(). Checking internal is_null flag..." << std::endl;
            bool is_truly_script_null = false;
            if (val_ref.operator->()) {
                is_truly_script_null = val_ref.operator->()->is_null; // Direct access to temporarily public member
                std::cerr << "[DEBUG IDENTIFIER]     Direct Value::is_null flag is: " << (is_truly_script_null ? "true" : "false") << std::endl;
            } else {
                std::cerr << "[DEBUG IDENTIFIER]     val_ref.operator->() is C++ NULL for variable '" << name_ << "'" << std::endl;
                is_truly_script_null = true; // Treat C++ null as effectively script null for error cause
            }
            std::cerr << "[DEBUG IDENTIFIER]   Attempting toString() for VARIABLE '" << name_ << "': " << val_ref.toString() << std::endl;
            std::cerr << "[DEBUG IDENTIFIER]   toString() succeeded for VARIABLE '" << name_ << "'." << std::endl;
            if (is_truly_script_null) { // Corrected variable name
                 std::cerr << "[DEBUG IDENTIFIER]   WARNING: Returning a SCRIPT-NULL ValuePtr for VARIABLE '" << name_ << "'" << std::endl;
            }
            return val_ref;
        }
        
        symbol_from_scope = sc->getConstant(name_); // Re-assign to the same variable
        if (symbol_from_scope) {
            std::cerr << "[DEBUG IDENTIFIER]   Found '" << name_ << "' as CONSTANT in scope. Accessing its value..." << std::endl;
            const auto& val_ref = symbol_from_scope->getValue(); // Triggers Symbol::getValue() log
            std::cerr << "[DEBUG IDENTIFIER]   ValuePtr for CONSTANT '" << name_ << "' obtained from getValue(). Checking internal is_null flag..." << std::endl;
            bool is_truly_script_null = false;
            if (val_ref.operator->()) {
                is_truly_script_null = val_ref.operator->()->is_null; // Direct access to temporarily public member
                std::cerr << "[DEBUG IDENTIFIER]     Direct Value::is_null flag is: " << (is_truly_script_null ? "true" : "false") << std::endl;
            } else {
                std::cerr << "[DEBUG IDENTIFIER]     val_ref.operator->() is C++ NULL for constant '" << name_ << "'" << std::endl;
                is_truly_script_null = true; // Treat C++ null as effectively script null for error cause
            }
            std::cerr << "[DEBUG IDENTIFIER]   Attempting toString() for CONSTANT '" << name_ << "': " << val_ref.toString() << std::endl;
            std::cerr << "[DEBUG IDENTIFIER]   toString() succeeded for CONSTANT '" << name_ << "'." << std::endl;
            if (is_truly_script_null) { // Corrected variable name
                 std::cerr << "[DEBUG IDENTIFIER]   WARNING: Returning a SCRIPT-NULL ValuePtr for CONSTANT '" << name_ << "'" << std::endl;
            }
            return val_ref;
        }

        // +++ Add New Logging for failure +++
        std::cerr << "[DEBUG IDENTIFIER]   Identifier '" << name_ << "' NOT FOUND as variable or constant in current scope." << std::endl;
        // +++ End New Logging +++

        throw Exception("Identifier '" + name_ + "' not found.", eval_filename, eval_line, eval_column);
        return nullptr;
    }

    std::string toString() const override { return name_; }
};

}  // namespace Interpreter

#endif  // IDENTIFIER_EXPRESSION_NODE_HPP
