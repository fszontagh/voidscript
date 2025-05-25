#ifndef SYMBOLS_FUNCTION_PARAMETER_INFO_HPP
#define SYMBOLS_FUNCTION_PARAMETER_INFO_HPP

#include <string>
#include "Symbols/VariableTypes.hpp"

namespace Parser {
    class ParsedExpression;
    using ParsedExpressionPtr = std::shared_ptr<ParsedExpression>;
}

namespace Symbols {

/**
 * @brief Documentation structure for function parameters.
 */
struct FunctionParameterInfo {
    std::string              name;                 // the name of the parameter
    Variables::Type          type;                 // the type of the parameter
    std::string              description;          // the description of the parameter
    bool                     optional    = false;  // if the parameter is optional
    bool                     interpolate = false;  // if the parameter is interpolated
};

}  // namespace Symbols

#endif
