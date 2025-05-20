#ifndef VARIABLE_TYPES_HPP
#define VARIABLE_TYPES_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>

namespace Symbols::Variables {

enum class Type : std::uint8_t { INTEGER, DOUBLE, FLOAT, STRING, BOOLEAN, OBJECT, CLASS, NULL_TYPE, UNDEFINED_TYPE };

const std::unordered_map<std::string, Type> StringToTypeMap = {
    { "int",       Type::INTEGER        },
    { "double",    Type::DOUBLE         },
    { "float",     Type::FLOAT          },
    { "string",    Type::STRING         },
    { "bool",      Type::BOOLEAN        },
    { "boolean",   Type::BOOLEAN        },
    { "null",      Type::NULL_TYPE      },
    { "object",    Type::OBJECT         },
    { "class",     Type::CLASS          },

    { "undefined", Type::UNDEFINED_TYPE },
};
const std::unordered_map<Type, std::string> StypeToStringMap = {
    { Type::INTEGER,        "int"        },
    { Type::DOUBLE,         "double"     },
    { Type::FLOAT,          "float"      },
    { Type::STRING,         "string"     },
    { Type::BOOLEAN,        "bool"       },
    { Type::OBJECT,         "object"     },
    { Type::CLASS,          "class"      },
    { Type::NULL_TYPE,      "null"       },
    { Type::UNDEFINED_TYPE, "undefined"  },
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
