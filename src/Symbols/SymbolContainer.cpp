#include "SymbolContainer.hpp" 
#include "Parser/ParsedExpression.hpp"  // For implementation methods using ParsedExpressionPtr

namespace Symbols {
    std::string SymbolContainer::initial_scope_name_for_singleton_;
    bool SymbolContainer::is_initialized_for_singleton_ = false;
} // namespace Symbols 