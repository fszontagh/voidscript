#ifndef VARIABLE_TYPES_HPP
#define VARIABLE_TYPES_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

namespace Symbols::Variables {

enum class Type : std::uint8_t { VT_INT, VT_DOUBLE, VT_STRING, VT_BOOLEAN, VT_NULL, VT_UNDEFINED };

const std::unordered_map<std::string, Type> StringToTypeMap = {
    { "int",       Type::VT_INT       },
    { "double",    Type::VT_DOUBLE    },
    { "string",    Type::VT_STRING    },
    { "bool",      Type::VT_BOOLEAN   },
    { "boolean",   Type::VT_BOOLEAN   },
    { "null",      Type::VT_NULL      },

    { "undefined", Type::VT_UNDEFINED },
};
const std::unordered_map<Type, std::string> StypeToStringMap = {
    { Type::VT_INT,       "int"        },
    { Type::VT_DOUBLE,    "double"     },
    { Type::VT_STRING,    "string"     },
    { Type::VT_BOOLEAN,   "bool"       },
    { Type::VT_NULL,      "null"       },
    { Type::VT_UNDEFINED, "undeffined" },
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

};  // namespace Symbols
#endif  // VARIABLE_TYPES_HPP
