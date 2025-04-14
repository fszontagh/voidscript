#ifndef VARIABLE_TYPES_HPP
#define VARIABLE_TYPES_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>

namespace Variables {

using DataContainer = std::variant<int, double, std::string, bool>;

enum class Type : std::uint8_t { VT_INT, VT_DOUBLE, VT_STRING, VT_BOOLEAN, VT_NULL, VT_NOT_DEFINED };

const std::unordered_map<std::string, Type> StringToTypeMap = {
    { "int",         Type::VT_INT         },
    { "double",      Type::VT_DOUBLE      },
    { "string",      Type::VT_STRING      },
    { "bool",        Type::VT_BOOLEAN     },
    { "boolean",     Type::VT_BOOLEAN     },
    { "null",        Type::VT_NULL        },
    { "not_defined", Type::VT_NOT_DEFINED },
};
const std::unordered_map<Type, std::string> StypeToStringMap = {
    { Type::VT_INT,         "int"         },
    { Type::VT_DOUBLE,      "double"      },
    { Type::VT_STRING,      "string"      },
    { Type::VT_BOOLEAN,     "bool"        },
    { Type::VT_NULL,        "null"        },
    { Type::VT_NOT_DEFINED, "not_defined" },
};

inline static std::string TypeToString(Type type) {
    if (StypeToStringMap.find(type) != StypeToStringMap.end()) {
        return StypeToStringMap.at(type);
    }
    return "null";
};

inline static Type StringToType(const std::string & type) {
    if (StringToTypeMap.find(type) != StringToTypeMap.end()) {
        return StringToTypeMap.at(type);
    }
    return Type::VT_NULL;
};

inline static std::string ToString(const DataContainer & data, const Type & type) {
    switch (type) {
        case Type::VT_INT:
            return std::to_string(std::get<int>(data));
        case Type::VT_DOUBLE:
            return std::to_string(std::get<double>(data));
        case Type::VT_STRING:
            return std::get<std::string>(data);
        case Type::VT_BOOLEAN:
            return std::get<bool>(data) ? "true" : "false";
        case Type::VT_NULL:
        default:
            return "null";
    }
};

};  // namespace Variables
#endif  // VARIABLE_TYPES_HPP
