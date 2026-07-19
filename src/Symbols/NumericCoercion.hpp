#ifndef SYMBOLS_NUMERIC_COERCION_HPP
#define SYMBOLS_NUMERIC_COERCION_HPP

#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Symbols {

/**
 * @brief Convert a numeric value to a target numeric type, in place.
 *
 * Every float literal lexes as double, so without this `float $f = 2.718;` is a type
 * error. Narrowing to int is deliberately NOT allowed: it would silently discard the
 * fractional part, which is the kind of thing a typed language should refuse.
 *
 * @return true if the value was converted (or already had the target type),
 *         false if the conversion is not permitted - the caller should report a
 *         type mismatch.
 */
inline bool tryNumericCoerce(ValuePtr & value, Variables::Type targetType) {
    const auto valueType = value.getType();
    if (valueType == targetType) {
        return true;
    }

    const bool valueIsNumeric = valueType == Variables::Type::INTEGER ||
                                valueType == Variables::Type::FLOAT ||
                                valueType == Variables::Type::DOUBLE;
    const bool targetIsFloating = targetType == Variables::Type::FLOAT ||
                                  targetType == Variables::Type::DOUBLE;

    if (!valueIsNumeric || !targetIsFloating) {
        return false;
    }

    const double raw = (valueType == Variables::Type::INTEGER) ? static_cast<double>(value.get<int>())
                       : (valueType == Variables::Type::FLOAT) ? static_cast<double>(value.get<float>())
                                                               : value.get<double>();

    value = (targetType == Variables::Type::FLOAT) ? ValuePtr(static_cast<float>(raw)) : ValuePtr(raw);
    return true;
}

}  // namespace Symbols

#endif  // SYMBOLS_NUMERIC_COERCION_HPP
