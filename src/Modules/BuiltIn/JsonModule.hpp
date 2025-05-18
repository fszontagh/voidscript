// JsonModule.hpp
#ifndef MODULES_JSONMODULE_HPP
#define MODULES_JSONMODULE_HPP

#include <cctype>
#include <stdexcept>
#include <string>
#include <variant>

#include "Modules/BaseModule.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

/**
 * @brief Module providing JSON encode/decode functions.
 *   json_encode(value) -> string
 *   json_decode(string) -> object/value
 */
class JsonModule : public BaseModule {
  public:
    JsonModule() { setModuleName("Json"); }

    void registerModule() override {
        std::vector<FunctParameterInfo> params = {
            { "object", Symbols::Variables::Type::OBJECT, "The object / array to serialize" },
        };
        REGISTER_FUNCTION("json_encode", Symbols::Variables::Type::STRING, params, "Serialize a value to JSON string",
                          [](const FunctionArguments & args) -> Symbols::ValuePtr {
                              using namespace Symbols;
                              if (args.size() != 1) {
                                  throw std::runtime_error("json_encode expects 1 argument");
                              }
                              // forward to encoder
                              std::function<std::string(Symbols::ValuePtr)> encode;
                              encode = [&](Symbols::ValuePtr v) -> std::string {
                                  const auto & var = v.raw()->get<std::string>();
                                  return std::visit(
                                      [&](auto && x) -> std::string {
                                          using T = std::decay_t<decltype(x)>;
                                          if constexpr (std::is_same_v<T, bool>) {
                                              return x ? "true" : "false";
                                          } else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double> ||
                                                               std::is_same_v<T, float>) {
                                              return std::to_string(x);
                                          } else if constexpr (std::is_same_v<T, std::string>) {
                                              // escape string
                                              std::string out = "\"";
                                              for (char c : x) {
                                                  switch (c) {
                                                      case '"':
                                                          out += "\\\"";
                                                          break;
                                                      case '\\':
                                                          out += "\\\\";
                                                          break;
                                                      case '\b':
                                                          out += "\\b";
                                                          break;
                                                      case '\f':
                                                          out += "\\f";
                                                          break;
                                                      case '\n':
                                                          out += "\\n";
                                                          break;
                                                      case '\r':
                                                          out += "\\r";
                                                          break;
                                                      case '\t':
                                                          out += "\\t";
                                                          break;
                                                      default:
                                                          if (static_cast<unsigned char>(c) < 0x20) {
                                                              // control character
                                                              char buf[7];
                                                              std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                                                              out += buf;
                                                          } else {
                                                              out += c;
                                                          }
                                                  }
                                              }
                                              out += "\"";
                                              return out;
                                          } else if constexpr (std::is_same_v<T, Symbols::ObjectMap>) {
                                              std::string out   = "{";
                                              bool        first = true;
                                              for (const auto & kv : x) {
                                                  if (!first) {
                                                      out += ",";
                                                  }
                                                  first = false;
                                                  // key
                                                  out += '"';
                                                  // escape key string
                                                  for (char c : kv.first) {
                                                      switch (c) {
                                                          case '"':
                                                              out += "\\\"";
                                                              break;
                                                          case '\\':
                                                              out += "\\\\";
                                                              break;
                                                          case '\b':
                                                              out += "\\b";
                                                              break;
                                                          case '\f':
                                                              out += "\\f";
                                                              break;
                                                          case '\n':
                                                              out += "\\n";
                                                              break;
                                                          case '\r':
                                                              out += "\\r";
                                                              break;
                                                          case '\t':
                                                              out += "\\t";
                                                              break;
                                                          default:
                                                              if (static_cast<unsigned char>(c) < 0x20) {
                                                                  char buf[7];
                                                                  std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                                                                  out += buf;
                                                              } else {
                                                                  out += c;
                                                              }
                                                      }
                                                  }
                                                  out += '"';
                                                  out += ':';
                                                  out += encode(kv.second);
                                              }
                                              out += "}";
                                              return out;
                                          } else {
                                              return "null";
                                          }
                                      },
                                      var);
                              };
                              std::string result = encode(args.at(0));
                              return Symbols::ValuePtr::create(result);
                          });

        params = {
            { "object", Symbols::Variables::Type::STRING, "The string to parse into object" },
        };
        REGISTER_FUNCTION("json_decode", Symbols::Variables::Type::OBJECT, params, "Parse JSON string into object",
                          &Modules::JsonModule::JsonDecode);
    }

    static Symbols::ValuePtr JsonDecode(const FunctionArguments & args) {
        if (args.size() != 1) {
            throw std::runtime_error("json_decode expects 1 argument");
        }
        if (args[0]->getType() != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("json_decode expects a JSON string");
        }
        const std::string s = args[0]->get<std::string>();

        struct Parser {
            const std::string & s;
            size_t              pos = 0;

            Parser(const std::string & str) : s(str), pos(0) {}

            void skip() {
                while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) {
                    pos++;
                }
            }

            std::string parseString() {
                skip();
                if (s[pos] != '"') {
                    throw std::runtime_error("Invalid JSON string");
                }
                pos++;
                std::string out;
                while (pos < s.size()) {
                    char c = s[pos++];
                    if (c == '"') {
                        break;
                    }
                    if (c == '\\') {
                        if (pos >= s.size()) {
                            break;
                        }
                        char e = s[pos++];
                        switch (e) {
                            case '"':
                                out += '"';
                                break;
                            case '\\':
                                out += '\\';
                                break;
                            case '/':
                                out += '/';
                                break;
                            case 'b':
                                out += '\b';
                                break;
                            case 'f':
                                out += '\f';
                                break;
                            case 'n':
                                out += '\n';
                                break;
                            case 'r':
                                out += '\r';
                                break;
                            case 't':
                                out += '\t';
                                break;
                            default:
                                out += e;
                                break;
                        }
                    } else {
                        out += c;
                    }
                }
                return out;
            }

            Symbols::ValuePtr parseNumber() {
                skip();
                size_t start = pos;
                if (s[pos] == '-') {
                    pos++;
                }
                while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
                    pos++;
                }
                bool isDouble = false;
                if (pos < s.size() && s[pos] == '.') {
                    isDouble = true;
                    pos++;
                    while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
                        pos++;
                    }
                }
                std::string num = s.substr(start, pos - start);
                try {
                    if (isDouble) {
                        return Symbols::ValuePtr::create(std::stod(num));
                    }
                    return Symbols::ValuePtr::create(std::stoi(num));
                } catch (...) {
                    throw std::runtime_error("Invalid JSON number: " + num);
                }
            }

            Symbols::ValuePtr parseBool() {
                skip();
                if (s.compare(pos, 4, "true") == 0) {
                    pos += 4;
                    return Symbols::ValuePtr::create(true);
                }
                if (s.compare(pos, 5, "false") == 0) {
                    pos += 5;
                    return Symbols::ValuePtr::create(false);
                }
                throw std::runtime_error("Invalid JSON boolean");
            }

            Symbols::ValuePtr parseNull() {
                skip();
                if (s.compare(pos, 4, "null") == 0) {
                    pos += 4;
                    return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
                }
                throw std::runtime_error("Invalid JSON null");
            }

            Symbols::ValuePtr parseObject() {
                skip();
                if (s[pos] != '{') {
                    throw std::runtime_error("Invalid JSON object");
                }
                pos++;
                skip();
                Symbols::ObjectMap obj;
                if (s[pos] == '}') {
                    pos++;
                    return Symbols::ValuePtr::create(obj);
                }
                while (pos < s.size()) {
                    skip();
                    std::string key = parseString();
                    skip();
                    if (s[pos] != ':') {
                        throw std::runtime_error("Expected ':' in object");
                    }
                    pos++;
                    skip();
                    Symbols::ValuePtr val = parseValue();
                    obj.emplace(key, val);
                    skip();
                    if (s[pos] == ',') {
                        pos++;
                        continue;
                    }
                    if (s[pos] == '}') {
                        pos++;
                        break;
                    }
                    throw std::runtime_error("Expected ',' or '}' in object");
                }
                return Symbols::ValuePtr::create(obj);
            }

            Symbols::ValuePtr parseValue() {
                skip();
                if (pos >= s.size()) {
                    throw std::runtime_error("Empty JSON");
                }
                char c = s[pos];
                if (c == '{') {
                    return parseObject();
                }
                if (c == '"') {
                    std::string str = parseString();
                    return Symbols::ValuePtr::create(str);
                }
                if (c == 't' || c == 'f') {
                    return parseBool();
                }
                if (c == 'n') {
                    return parseNull();
                }
                if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
                    return parseNumber();
                }
                throw std::runtime_error(std::string("Invalid JSON value at pos ") + std::to_string(pos));
            }
        } parser(s);

        Symbols::ValuePtr result = parser.parseValue();
        return result;
    }
};
};  // namespace Modules
#endif  // MODULES_JSONMODULE_HPP
