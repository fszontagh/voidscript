#ifndef INTERPRETER_ENUM_ACCESS_EXPRESSION_NODE_HPP
#define INTERPRETER_ENUM_ACCESS_EXPRESSION_NODE_HPP

#include <memory>
#include <string>

#include "Interpreter/ExpressionNode.hpp"
#include "Interpreter/Interpreter.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/EnumSymbol.hpp"
#include "Symbols/Value.hpp"

namespace Interpreter {

// Expression node for enum value access: EnumName.VALUE
class EnumAccessExpressionNode : public ExpressionNode {
  public:
    EnumAccessExpressionNode(std::string enumName, std::string valueName,
                           const std::string & filename, int line, size_t column) :
        enumName_(std::move(enumName)),
        valueName_(std::move(valueName)),
        filename_(filename),
        line_(line),
        column_(column) {}

    Symbols::ValuePtr evaluate(Interpreter & interpreter, std::string /*filename*/, int /*line*/,
                               size_t /*col */) const override {

        auto* symbolContainer = Symbols::SymbolContainer::instance();
        
        // Find the enum symbol using the getEnum method which handles scope traversal
        auto enumSymbol = symbolContainer->getEnum(enumName_);

        if (!enumSymbol) {
            throw Exception("Enum '" + enumName_ + "' not found", filename_, line_, column_);
        }

        // Cast to EnumSymbol and get the value
        auto enumSymbolCast = std::dynamic_pointer_cast<Symbols::EnumSymbol>(enumSymbol);
        if (!enumSymbolCast) {
            throw Exception("Symbol '" + enumName_ + "' is not an enum", filename_, line_, column_);
        }

        auto enumValue = enumSymbolCast->GetValue(valueName_);
        if (!enumValue.has_value()) {
            throw Exception("Enum value '" + valueName_ + "' not found in enum '" + enumName_ + "'", 
                          filename_, line_, column_);
        }

        // Return the enum value as an integer
        return Symbols::ValuePtr(enumValue.value());
    }

    std::string toString() const override { 
        return enumName_ + "." + valueName_; 
    }

  private:
    std::string enumName_;
    std::string valueName_;
    std::string filename_;
    int         line_;
    size_t      column_;
};

}  // namespace Interpreter

#endif  // INTERPRETER_ENUM_ACCESS_EXPRESSION_NODE_HPP