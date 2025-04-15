#ifndef VARIABLE_TYPES_HPP
#define VARIABLE_TYPES_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

namespace Symbols::Variables {

enum class Type : std::uint8_t { INTEGER, DOUBLE, FLOAT, STRING, BOOLEAN, NULL_TYPE, UNDEFINED_TYPE };

const std::unordered_map<std::string, Type> StringToTypeMap = {
    { "int",       Type::INTEGER        },
    { "double",    Type::DOUBLE         },
    { "float",     Type::FLOAT          },
    { "string",    Type::STRING         },
    { "bool",      Type::BOOLEAN        },
    { "boolean",   Type::BOOLEAN        },
    { "null",      Type::NULL_TYPE      },

    { "undefined", Type::UNDEFINED_TYPE },
};
const std::unordered_map<Type, std::string> StypeToStringMap = {
    { Type::INTEGER,        "int"        },
    { Type::DOUBLE,         "double"     },
    { Type::FLOAT,          "float"      },
    { Type::STRING,         "string"     },
    { Type::BOOLEAN,        "bool"       },
    { Type::NULL_TYPE,      "null"       },
    { Type::UNDEFINED_TYPE, "undeffined" },
};

inline static std::string TypeToString(Symbols::Variables::Type type) {
    if (StypeToStringMap.find(type) != StypeToStringMap.end()) {
        return StypeToStringMap.at(type);
    }
    return "null";
};

inline static Type StringToType(const std::string & type) {
    if (StringToTypeMap.find(type) != StringToTypeMap.end()) {
        return StringToTypeMap.at(type);
    }
    return Type::NULL_TYPE;
};

};  // namespace Symbols::Variables
#endif  // VARIABLE_TYPES_HPP
