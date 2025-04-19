#ifndef SYMBOLS_PARAMETER_CONTAINER_HPP
#define SYMBOLS_PARAMETER_CONTAINER_HPP

#include <string>
#include <vector>

#include "Symbols/VariableTypes.hpp"

namespace Symbols {
struct functionParameterType {
    std::string              name;
    Symbols::Variables::Type type;
};

using FunctionParameterInfo = std::vector<Symbols::functionParameterType>;
}  // namespace Symbols
#endif
