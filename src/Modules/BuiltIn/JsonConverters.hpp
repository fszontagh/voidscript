// JsonConverters.hpp
#ifndef MODULES_JSONCONVERTERS_HPP
#define MODULES_JSONCONVERTERS_HPP

#include "../../json.hpp"
#include "../../Symbols/Value.hpp"
#include "../../Symbols/VariableTypes.hpp"
#include <stdexcept>
#include <string>
#include <map>
#include <vector>
#include <memory>

namespace Modules {

/**
 * @brief Namespace containing conversion functions between VoidScript ValuePtr and nlohmann::json objects
 */
namespace JsonConverters {

/**
 * @brief Converts a VoidScript ValuePtr to nlohmann::json
 *
 * @param value The VoidScript ValuePtr to convert
 * @return nlohmann::json The converted JSON object
 * @throws std::runtime_error If conversion fails due to unsupported type or error
 */
nlohmann::json valueToJson(const Symbols::ValuePtr& value);

/**
 * @brief Converts an nlohmann::json to VoidScript ValuePtr
 *
 * @param json The nlohmann::json object to convert
 * @return Symbols::ValuePtr The converted VoidScript value
 * @throws std::runtime_error If conversion fails due to unsupported type or error
 */
Symbols::ValuePtr jsonToValue(const nlohmann::json& json);

/**
 * @brief Converts a VoidScript ValuePtr to nlohmann::json with error context
 *
 * @param value The VoidScript ValuePtr to convert
 * @param context Error context string for better error messages
 * @return nlohmann::json The converted JSON object
 * @throws std::runtime_error If conversion fails with detailed error information
 */
nlohmann::json valueToJsonWithContext(const Symbols::ValuePtr& value, const std::string& context = "");

/**
 * @brief Converts an nlohmann::json to VoidScript ValuePtr with error context
 *
 * @param json The nlohmann::json object to convert
 * @param context Error context string for better error messages
 * @return Symbols::ValuePtr The converted VoidScript value
 * @throws std::runtime_error If conversion fails with detailed error information
 */
Symbols::ValuePtr jsonToValueWithContext(const nlohmann::json& json, const std::string& context = "");

/**
 * @brief Validates if a VoidScript ValuePtr can be converted to JSON
 *
 * @param value The ValuePtr to validate
 * @return true If the value can be converted to JSON
 * @return false If the value cannot be converted to JSON
 */
bool canConvertToJson(const Symbols::ValuePtr& value);

/**
 * @brief Validates if an nlohmann::json can be converted to VoidScript ValuePtr
 *
 * @param json The JSON object to validate
 * @return true If the JSON can be converted to VoidScript
 * @return false If the JSON cannot be converted to VoidScript
 */
bool canConvertToValue(const nlohmann::json& json);

} // namespace JsonConverters

} // namespace Modules

#endif // MODULES_JSONCONVERTERS_HPP