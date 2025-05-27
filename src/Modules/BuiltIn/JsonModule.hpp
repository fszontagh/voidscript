// JsonModule.hpp
#ifndef MODULES_JSONMODULE_HPP
#define MODULES_JSONMODULE_HPP

#include <cctype>
#include <stdexcept>
#include <string>
#include <variant>

#include "Modules/BaseModule.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"
#include "Symbols/RegistrationMacros.hpp"

namespace Modules {

/**
 * @brief Module providing JSON encode/decode functions.
 *   json_encode(value) -> string
 *   json_decode(string) -> object/value
 */
class JsonModule : public BaseModule {
  public:
    JsonModule() { setModuleName("Json"); }

    void registerFunctions() override {
        std::vector<Symbols::FunctionParameterInfo> params = {
            { "object", Symbols::Variables::Type::OBJECT, "The object / array to serialize", false, false },
        };

        REGISTER_FUNCTION("json_encode", Symbols::Variables::Type::STRING, params, "Serialize a value to JSON string",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw std::runtime_error("json_encode expects 1 argument");
                              }

                              // forward-declared lambda
                              std::function<std::string(const Symbols::ValuePtr &)> encode;

                              encode = [&](const Symbols::ValuePtr & v) -> std::string {
                                  using Symbols::Variables::Type;

                                  switch (v.getType()) {
                                      case Type::NULL_TYPE:
                                          return "null";

                                      case Type::BOOLEAN:
                                          return v.get<bool>() ? "true" : "false";

                                      case Type::INTEGER:
                                          return std::to_string(v.get<int>());

                                      case Type::FLOAT:
                                          return std::to_string(v.get<float>());

                                      case Type::DOUBLE:
                                          return std::to_string(v.get<double>());

                                      case Type::STRING:
                                          {
                                              const std::string & x   = v.get<std::string>();
                                              std::string         out = "\"";
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
                                          }

                                      case Type::OBJECT:
                                          {
                                              const auto & obj   = v.get<Symbols::ObjectMap>();
                                              std::string  out   = "{";
                                              bool         first = true;
                                              for (const auto & kv : obj) {
                                                  if (!first) {
                                                      out += ",";
                                                  }
                                                  first = false;
                                                  out += "\"";
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
                                                  out += "\":";
                                                  out += encode(kv.second);
                                              }
                                              out += "}";
                                              return out;
                                          }

                                      default:
                                          return "null";  // fallback
                                  }
                              };

                              return encode(args.at(0));
                          });

        params = {
            { "object", Symbols::Variables::Type::STRING, "The string to parse into object", false, false },
        };
        REGISTER_FUNCTION("json_decode", Symbols::Variables::Type::OBJECT, params, "Parse JSON string into object",
                          &Modules::JsonModule::JsonDecode);
    }

    static Symbols::ValuePtr JsonDecode(const Symbols::FunctionArguments & args) {
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
                        return Symbols::ValuePtr(std::stod(num));
                    }
                    return Symbols::ValuePtr(std::stoi(num));
                } catch (...) {
                    throw std::runtime_error("Invalid JSON number: " + num);
                }
            }

            Symbols::ValuePtr parseBool() {
                skip();
                if (s.compare(pos, 4, "true") == 0) {
                    pos += 4;
                    return Symbols::ValuePtr(true);
                }
                if (s.compare(pos, 5, "false") == 0) {
                    pos += 5;
                    return Symbols::ValuePtr(false);
                }
                throw std::runtime_error("Invalid JSON boolean");
            }

            Symbols::ValuePtr parseNull() {
                skip();
                if (s.compare(pos, 4, "null") == 0) {
                    pos += 4;
                    return Symbols::ValuePtr::null(Symbols::Variables::Type::NULL_TYPE);
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
                    return Symbols::ValuePtr(obj);
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
                return Symbols::ValuePtr(obj);
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
                    return Symbols::ValuePtr(str);
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
