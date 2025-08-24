// JsonConverters.cpp
#include "JsonConverters.hpp"
#include "../../json.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <limits>

namespace Modules {
namespace JsonConverters {

namespace {

/**
 * @brief Helper function to create detailed error messages
 *
 * @param operation The operation being performed (e.g., "ValuePtr to JSON")
 * @param valueType The type of the ValuePtr
 * @param context Additional context information
 * @return std::string Detailed error message
 */
std::string createErrorMessage(const std::string& operation,
                              Symbols::Variables::Type valueType,
                              const std::string& context = "") {
    std::string typeStr = Symbols::Variables::TypeToString(valueType);
    std::string msg = "Conversion error in " + operation +
                     ": unsupported type '" + typeStr + "'";

    if (!context.empty()) {
        msg += " in context: " + context;
    }

    return msg;
}

/**
 * @brief Helper function to create detailed error messages for JSON conversion
 *
 * @param operation The operation being performed (e.g., "JSON to ValuePtr")
 * @param jsonType The type of the JSON value
 * @param context Additional context information
 * @return std::string Detailed error message
 */
std::string createJsonErrorMessage(const std::string& operation,
                                  const std::string& jsonType,
                                  const std::string& context = "") {
    std::string msg = "Conversion error in " + operation +
                     ": unsupported JSON type '" + jsonType + "'";

    if (!context.empty()) {
        msg += " in context: " + context;
    }

    return msg;
}

/**
 * @brief Converts a JSON number to the appropriate VoidScript numeric type
 *
 * @param json The JSON number value
 * @return Symbols::ValuePtr The converted numeric ValuePtr
 */
Symbols::ValuePtr convertJsonNumberToValue(const nlohmann::json& json) {
    if (json.is_number_integer()) {
        return Symbols::ValuePtr(static_cast<int>(json.get<int64_t>()));
    } else if (json.is_number_unsigned()) {
        return Symbols::ValuePtr(static_cast<int>(json.get<uint64_t>()));
    } else if (json.is_number_float()) {
        double value = json.get<double>();
        // Check if it can be represented as a float without losing precision
        if (value >= std::numeric_limits<float>::min() &&
            value <= std::numeric_limits<float>::max() &&
            std::abs(value) <= std::numeric_limits<float>::max()) {
            return Symbols::ValuePtr(static_cast<float>(value));
        }
        return Symbols::ValuePtr(value);
    }

    throw std::runtime_error("Invalid JSON number type");
}

/**
 * @brief Recursively converts a JSON object to VoidScript ObjectMap
 *
 * @param json The JSON object to convert
 * @return Symbols::ObjectMap The converted ObjectMap
 */
Symbols::ObjectMap convertJsonObjectToMap(const nlohmann::json& json) {
    Symbols::ObjectMap result;

    for (auto it = json.begin(); it != json.end(); ++it) {
        const std::string& key = it.key();
        const nlohmann::json& value = it.value();

        if (value.is_null()) {
            result[key] = Symbols::ValuePtr::null(Symbols::Variables::Type::NULL_TYPE);
        } else if (value.is_boolean()) {
            result[key] = Symbols::ValuePtr(value.get<bool>());
        } else if (value.is_number_integer()) {
            result[key] = Symbols::ValuePtr(static_cast<int>(value.get<int64_t>()));
        } else if (value.is_number_unsigned()) {
            result[key] = Symbols::ValuePtr(static_cast<int>(value.get<uint64_t>()));
        } else if (value.is_number_float()) {
            result[key] = Symbols::ValuePtr(value.get<double>());
        } else if (value.is_string()) {
            result[key] = Symbols::ValuePtr(value.get<std::string>());
        } else if (value.is_array()) {
            // Convert JSON array to ObjectMap with numeric string keys
            Symbols::ObjectMap arrayMap;
            for (size_t i = 0; i < value.size(); ++i) {
                const std::string indexKey = std::to_string(i);
                const nlohmann::json& element = value[i];

                if (element.is_null()) {
                    arrayMap[indexKey] = Symbols::ValuePtr::null(Symbols::Variables::Type::NULL_TYPE);
                } else if (element.is_boolean()) {
                    arrayMap[indexKey] = Symbols::ValuePtr(element.get<bool>());
                } else if (element.is_number_integer()) {
                    arrayMap[indexKey] = Symbols::ValuePtr(static_cast<int>(element.get<int64_t>()));
                } else if (element.is_number_unsigned()) {
                    arrayMap[indexKey] = Symbols::ValuePtr(static_cast<int>(element.get<uint64_t>()));
                } else if (element.is_number_float()) {
                    arrayMap[indexKey] = Symbols::ValuePtr(element.get<double>());
                } else if (element.is_string()) {
                    arrayMap[indexKey] = Symbols::ValuePtr(element.get<std::string>());
                } else if (element.is_object()) {
                    arrayMap[indexKey] = Symbols::ValuePtr(convertJsonObjectToMap(element));
                } else if (element.is_array()) {
                    arrayMap[indexKey] = Symbols::ValuePtr(convertJsonObjectToMap(element));
                }
            }
            result[key] = Symbols::ValuePtr(arrayMap);
        } else if (value.is_object()) {
            result[key] = Symbols::ValuePtr(convertJsonObjectToMap(value));
        }
    }

    return result;
}

/**
 * @brief Recursively converts a VoidScript ObjectMap to JSON object
 *
 * @param objMap The ObjectMap to convert
 * @return nlohmann::json The converted JSON object
 */
nlohmann::json convertMapToJson(const Symbols::ObjectMap& objMap) {
    nlohmann::json result;

    for (const auto& kv : objMap) {
        const std::string& key = kv.first;
        const Symbols::ValuePtr& value = kv.second;

        switch (value.getType()) {
            case Symbols::Variables::Type::NULL_TYPE:
                result[key] = nullptr;
                break;

            case Symbols::Variables::Type::BOOLEAN:
                result[key] = value.get<bool>();
                break;

            case Symbols::Variables::Type::INTEGER:
                result[key] = value.get<int>();
                break;

            case Symbols::Variables::Type::FLOAT:
                result[key] = value.get<float>();
                break;

            case Symbols::Variables::Type::DOUBLE:
                result[key] = value.get<double>();
                break;

            case Symbols::Variables::Type::STRING:
                result[key] = value.get<std::string>();
                break;

            case Symbols::Variables::Type::OBJECT:
            case Symbols::Variables::Type::CLASS:
                result[key] = convertMapToJson(value.get<Symbols::ObjectMap>());
                break;

            case Symbols::Variables::Type::ENUM:
                // Convert enum to string representation
                result[key] = value.toString();
                break;

            default:
                // Unsupported type, convert to null
                result[key] = nullptr;
                break;
        }
    }

    return result;
}

} // anonymous namespace

nlohmann::json valueToJson(const Symbols::ValuePtr& value) {
    if (!value) {
        throw std::runtime_error("Cannot convert null ValuePtr to JSON");
    }

    return valueToJsonWithContext(value, "");
}

Symbols::ValuePtr jsonToValue(const nlohmann::json& json) {
    return jsonToValueWithContext(json, "");
}

nlohmann::json valueToJsonWithContext(const Symbols::ValuePtr& value, const std::string& context) {
    if (!value) {
        throw std::runtime_error(createErrorMessage("ValuePtr to JSON conversion",
                                                  Symbols::Variables::Type::NULL_TYPE, context));
    }

    Symbols::Variables::Type type = value.getType();

    switch (type) {
        case Symbols::Variables::Type::NULL_TYPE:
            return nullptr;

        case Symbols::Variables::Type::BOOLEAN:
            return value.get<bool>();

        case Symbols::Variables::Type::INTEGER:
            return value.get<int>();

        case Symbols::Variables::Type::FLOAT:
            return value.get<float>();

        case Symbols::Variables::Type::DOUBLE:
            return value.get<double>();

        case Symbols::Variables::Type::STRING:
            return value.get<std::string>();

        case Symbols::Variables::Type::OBJECT:
        case Symbols::Variables::Type::CLASS:
            return convertMapToJson(value.get<Symbols::ObjectMap>());

        case Symbols::Variables::Type::ENUM:
            return value.toString();

        default:
            throw std::runtime_error(createErrorMessage("ValuePtr to JSON conversion", type, context));
    }
}

Symbols::ValuePtr jsonToValueWithContext(const nlohmann::json& json, const std::string& context) {
    if (json.is_null()) {
        return Symbols::ValuePtr::null(Symbols::Variables::Type::NULL_TYPE);
    } else if (json.is_boolean()) {
        return Symbols::ValuePtr(json.get<bool>());
    } else if (json.is_number_integer()) {
        return Symbols::ValuePtr(static_cast<int>(json.get<int64_t>()));
    } else if (json.is_number_unsigned()) {
        return Symbols::ValuePtr(static_cast<int>(json.get<uint64_t>()));
    } else if (json.is_number_float()) {
        return Symbols::ValuePtr(json.get<double>());
    } else if (json.is_string()) {
        return Symbols::ValuePtr(json.get<std::string>());
    } else if (json.is_array()) {
        // Convert JSON array to ObjectMap with numeric string keys
        Symbols::ObjectMap arrayMap;
        for (size_t i = 0; i < json.size(); ++i) {
            const std::string indexKey = std::to_string(i);
            const nlohmann::json& element = json[i];

            if (element.is_null()) {
                arrayMap[indexKey] = Symbols::ValuePtr::null(Symbols::Variables::Type::NULL_TYPE);
            } else if (element.is_boolean()) {
                arrayMap[indexKey] = Symbols::ValuePtr(element.get<bool>());
            } else if (element.is_number_integer()) {
                arrayMap[indexKey] = Symbols::ValuePtr(static_cast<int>(element.get<int64_t>()));
            } else if (element.is_number_unsigned()) {
                arrayMap[indexKey] = Symbols::ValuePtr(static_cast<int>(element.get<uint64_t>()));
            } else if (element.is_number_float()) {
                arrayMap[indexKey] = Symbols::ValuePtr(element.get<double>());
            } else if (element.is_string()) {
                arrayMap[indexKey] = Symbols::ValuePtr(element.get<std::string>());
            } else if (element.is_object()) {
                arrayMap[indexKey] = Symbols::ValuePtr(convertJsonObjectToMap(element));
            } else if (element.is_array()) {
                arrayMap[indexKey] = Symbols::ValuePtr(convertJsonObjectToMap(element));
            }
        }
        return Symbols::ValuePtr(arrayMap);
    } else if (json.is_object()) {
        return Symbols::ValuePtr(convertJsonObjectToMap(json));
    } else {
        throw std::runtime_error(createJsonErrorMessage("JSON to ValuePtr conversion",
                                                       "unknown", context));
    }
}

bool canConvertToJson(const Symbols::ValuePtr& value) {
    if (!value) {
        return false;
    }

    Symbols::Variables::Type type = value.getType();

    switch (type) {
        case Symbols::Variables::Type::NULL_TYPE:
        case Symbols::Variables::Type::BOOLEAN:
        case Symbols::Variables::Type::INTEGER:
        case Symbols::Variables::Type::FLOAT:
        case Symbols::Variables::Type::DOUBLE:
        case Symbols::Variables::Type::STRING:
        case Symbols::Variables::Type::OBJECT:
        case Symbols::Variables::Type::CLASS:
        case Symbols::Variables::Type::ENUM:
            return true;

        default:
            return false;
    }
}

bool canConvertToValue(const nlohmann::json& json) {
    return json.is_null() || json.is_boolean() || json.is_number() ||
           json.is_string() || json.is_array() || json.is_object();
}

} // namespace JsonConverters
} // namespace Modules