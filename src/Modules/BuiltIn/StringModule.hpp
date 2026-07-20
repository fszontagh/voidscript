#ifndef MODULES_STRINGMODULE_HPP
#define MODULES_STRINGMODULE_HPP

#include <algorithm>
#include <cctype>
#include <string>

#include "Modules/BaseModule.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/RegistrationMacros.hpp"

namespace Modules {

class StringModule : public BaseModule {
  public:
    StringModule() {
        setModuleName("String");
        setDescription("Provides string manipulation and processing functions including length calculation, replacement, and substring extraction");
        setBuiltIn(true);
    }

    void registerFunctions() override {
        // string_length
        std::vector<Symbols::FunctionParameterInfo> param_list = {
            { "string", Symbols::Variables::Type::STRING, "The string to calculate the length of", false, false }
        };

        REGISTER_FUNCTION("string_length", Symbols::Variables::Type::INTEGER, param_list,
                          "Calculate the length of a string", [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                                  throw Exception(name() + "::string_length expects one string argument");
                              }
                              const std::string str  = args[0];
                              size_t            size = str.size();
                              return static_cast<int>(size);
                          });


        // ---- Added: the module previously had only length/replace/substr, which forced
        // scripts into workarounds such as testing for a substring by replacing it and
        // comparing lengths. ----

        const auto requireString = [this](Symbols::FunctionArguments & args, size_t n, const char * fn) {
            if (args.size() != n) {
                throw Exception(name() + "::" + fn + " expects " + std::to_string(n) + " argument(s)");
            }
            for (size_t i = 0; i < n; ++i) {
                if (i == 1 && std::string(fn) == "string_repeat") {
                    continue;  // second argument is the count
                }
                if (args[i] != Symbols::Variables::Type::STRING) {
                    throw Exception(name() + "::" + fn + " expects string argument(s)");
                }
            }
        };

        std::vector<Symbols::FunctionParameterInfo> one_string = {
            { "string", Symbols::Variables::Type::STRING, "Input string", false, false }
        };
        std::vector<Symbols::FunctionParameterInfo> two_strings = {
            { "string", Symbols::Variables::Type::STRING, "Input string", false, false },
            { "other",  Symbols::Variables::Type::STRING, "Second string", false, false }
        };

        REGISTER_FUNCTION("string_to_upper", Symbols::Variables::Type::STRING, one_string,
                          "Convert a string to upper case", [=](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              requireString(args, 1, "string_to_upper");
                              std::string s = args[0];
                              std::transform(s.begin(), s.end(), s.begin(),
                                             [](unsigned char c) { return std::toupper(c); });
                              return s;
                          });

        REGISTER_FUNCTION("string_to_lower", Symbols::Variables::Type::STRING, one_string,
                          "Convert a string to lower case", [=](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              requireString(args, 1, "string_to_lower");
                              std::string s = args[0];
                              std::transform(s.begin(), s.end(), s.begin(),
                                             [](unsigned char c) { return std::tolower(c); });
                              return s;
                          });

        REGISTER_FUNCTION("string_trim", Symbols::Variables::Type::STRING, one_string,
                          "Remove leading and trailing whitespace", [=](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              requireString(args, 1, "string_trim");
                              const std::string s     = args[0];
                              const auto        first = s.find_first_not_of(" \t\n\r\f\v");
                              if (first == std::string::npos) {
                                  return std::string{};
                              }
                              const auto last = s.find_last_not_of(" \t\n\r\f\v");
                              return s.substr(first, last - first + 1);
                          });

        REGISTER_FUNCTION("string_reverse", Symbols::Variables::Type::STRING, one_string,
                          "Reverse a string", [=](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              requireString(args, 1, "string_reverse");
                              std::string s = args[0];
                              std::reverse(s.begin(), s.end());
                              return s;
                          });

        REGISTER_FUNCTION("string_index_of", Symbols::Variables::Type::INTEGER, two_strings,
                          "Index of the first occurrence of a substring, or -1",
                          [=](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              requireString(args, 2, "string_index_of");
                              const std::string hay    = args[0];
                              const std::string needle = args[1];
                              const auto        pos    = hay.find(needle);
                              return pos == std::string::npos ? -1 : static_cast<int>(pos);
                          });

        REGISTER_FUNCTION("string_contains", Symbols::Variables::Type::BOOLEAN, two_strings,
                          "Whether a string contains a substring",
                          [=](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              requireString(args, 2, "string_contains");
                              const std::string hay    = args[0];
                              const std::string needle = args[1];
                              return hay.find(needle) != std::string::npos;
                          });

        REGISTER_FUNCTION("string_starts_with", Symbols::Variables::Type::BOOLEAN, two_strings,
                          "Whether a string starts with a prefix",
                          [=](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              requireString(args, 2, "string_starts_with");
                              const std::string s      = args[0];
                              const std::string prefix = args[1];
                              return s.rfind(prefix, 0) == 0;
                          });

        REGISTER_FUNCTION("string_ends_with", Symbols::Variables::Type::BOOLEAN, two_strings,
                          "Whether a string ends with a suffix",
                          [=](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              requireString(args, 2, "string_ends_with");
                              const std::string s      = args[0];
                              const std::string suffix = args[1];
                              return s.size() >= suffix.size() &&
                                     s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
                          });

        param_list = {
            { "string", Symbols::Variables::Type::STRING,  "String to repeat", false, false },
            { "count",  Symbols::Variables::Type::INTEGER, "Number of repetitions", false, false }
        };
        REGISTER_FUNCTION("string_repeat", Symbols::Variables::Type::STRING, param_list,
                          "Repeat a string n times", [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 2 || args[0] != Symbols::Variables::Type::STRING ||
                                  args[1] != Symbols::Variables::Type::INTEGER) {
                                  throw Exception(name() + "::string_repeat expects (string, int)");
                              }
                              const std::string s     = args[0];
                              const int         count = args[1];
                              if (count < 0) {
                                  throw Exception(name() + "::string_repeat count must not be negative");
                              }
                              std::string out;
                              out.reserve(s.size() * static_cast<size_t>(count));
                              for (int i = 0; i < count; ++i) {
                                  out += s;
                              }
                              return out;
                          });

        REGISTER_FUNCTION("string_split", Symbols::Variables::Type::OBJECT, two_strings,
                          "Split a string on a separator, returning an array",
                          [=](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              requireString(args, 2, "string_split");
                              const std::string s   = args[0];
                              const std::string sep = args[1];
                              if (sep.empty()) {
                                  throw Exception(name() + "::string_split separator must not be empty");
                              }
                              // Arrays are ObjectMaps keyed by the decimal index.
                              Symbols::ObjectMap out;
                              size_t             idx   = 0;
                              size_t             start = 0;
                              size_t             hit   = 0;
                              while ((hit = s.find(sep, start)) != std::string::npos) {
                                  out[std::to_string(idx++)] = Symbols::ValuePtr(s.substr(start, hit - start));
                                  start                      = hit + sep.size();
                              }
                              out[std::to_string(idx)] = Symbols::ValuePtr(s.substr(start));
                              return Symbols::ValuePtr(out);
                          });

        param_list = {
            { "parts",     Symbols::Variables::Type::OBJECT, "Array of strings", false, false },
            { "separator", Symbols::Variables::Type::STRING, "Separator", false, false }
        };
        REGISTER_FUNCTION("string_join", Symbols::Variables::Type::STRING, param_list,
                          "Join an array of strings with a separator",
                          [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 2 || args[0] != Symbols::Variables::Type::OBJECT ||
                                  args[1] != Symbols::Variables::Type::STRING) {
                                  throw Exception(name() + "::string_join expects (array, string)");
                              }
                              const Symbols::ObjectMap parts = args[0];
                              const std::string        sep   = args[1];
                              std::string              out;
                              // Walk by index so the original order is preserved: the map
                              // is keyed by decimal index and sorts lexicographically.
                              for (size_t i = 0; i < parts.size(); ++i) {
                                  auto it = parts.find(std::to_string(i));
                                  if (it == parts.end()) {
                                      break;
                                  }
                                  if (i > 0) {
                                      out += sep;
                                  }
                                  out += it->second->toString();
                              }
                              return out;
                          });


        // ---- base64. Previously absent everywhere, so scripts had to shell out to the
        // base64 binary via process_run. ----
        REGISTER_FUNCTION("base64_encode", Symbols::Variables::Type::STRING, one_string,
                          "Base64-encode a string", [=](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              requireString(args, 1, "base64_encode");
                              static const char * tbl =
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
                              const std::string in = args[0];
                              std::string       out;
                              out.reserve(((in.size() + 2) / 3) * 4);
                              size_t i = 0;
                              while (i + 2 < in.size()) {
                                  const unsigned v = (static_cast<unsigned char>(in[i]) << 16) |
                                                     (static_cast<unsigned char>(in[i + 1]) << 8) |
                                                     static_cast<unsigned char>(in[i + 2]);
                                  out += tbl[(v >> 18) & 0x3F];
                                  out += tbl[(v >> 12) & 0x3F];
                                  out += tbl[(v >> 6) & 0x3F];
                                  out += tbl[v & 0x3F];
                                  i += 3;
                              }
                              const size_t rest = in.size() - i;
                              if (rest == 1) {
                                  const unsigned v = static_cast<unsigned char>(in[i]) << 16;
                                  out += tbl[(v >> 18) & 0x3F];
                                  out += tbl[(v >> 12) & 0x3F];
                                  out += "==";
                              } else if (rest == 2) {
                                  const unsigned v = (static_cast<unsigned char>(in[i]) << 16) |
                                                     (static_cast<unsigned char>(in[i + 1]) << 8);
                                  out += tbl[(v >> 18) & 0x3F];
                                  out += tbl[(v >> 12) & 0x3F];
                                  out += tbl[(v >> 6) & 0x3F];
                                  out += '=';
                              }
                              return out;
                          });

        REGISTER_FUNCTION("base64_decode", Symbols::Variables::Type::STRING, one_string,
                          "Decode a base64 string", [=](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              requireString(args, 1, "base64_decode");
                              const std::string in = args[0];
                              auto              val = [](char c) -> int {
                                  if (c >= 'A' && c <= 'Z') {
                                      return c - 'A';
                                  }
                                  if (c >= 'a' && c <= 'z') {
                                      return c - 'a' + 26;
                                  }
                                  if (c >= '0' && c <= '9') {
                                      return c - '0' + 52;
                                  }
                                  if (c == '+') {
                                      return 62;
                                  }
                                  if (c == '/') {
                                      return 63;
                                  }
                                  return -1;
                              };
                              std::string out;
                              unsigned    buf  = 0;
                              int         bits = 0;
                              for (char c : in) {
                                  if (c == '=' || std::isspace(static_cast<unsigned char>(c))) {
                                      continue;
                                  }
                                  const int d = val(c);
                                  if (d < 0) {
                                      throw Exception(name() + "::base64_decode: invalid base64 character");
                                  }
                                  buf = (buf << 6) | static_cast<unsigned>(d);
                                  bits += 6;
                                  if (bits >= 8) {
                                      bits -= 8;
                                      out += static_cast<char>((buf >> bits) & 0xFF);
                                  }
                              }
                              return out;
                          });

        // string_replace
        param_list = {
            { "string", Symbols::Variables::Type::STRING, "The string in which to replace", false, false },
            { "from",   Symbols::Variables::Type::STRING, "The string to replace from", false, false },
            { "to",     Symbols::Variables::Type::STRING, "The string to replace to", false, false }
        };
        REGISTER_FUNCTION("string_replace", Symbols::Variables::Type::STRING, param_list,
                          "Replace part of a string with another string",
                          [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() < 3) {
                                  throw Exception(name() + "::string_replace expects at least 3 arguments");
                              }
                              std::string str  = args[0];
                              std::string from = args[1];
                              std::string to   = args[2];
                              size_t      pos  = str.find(from);
                              if (pos != std::string::npos) {
                                  str.replace(pos, from.length(), to);
                              }
                              return str;
                          });

        // string_substr
        param_list = {
            { "string", Symbols::Variables::Type::STRING,  "The string to extract a substring from" },
            { "start",  Symbols::Variables::Type::INTEGER, "The start index of the substring"       },
            { "length", Symbols::Variables::Type::INTEGER, "The length of the substring"            }
        };
        REGISTER_FUNCTION("string_substr", Symbols::Variables::Type::STRING, param_list,
                          "Extract a substring from a string", [this](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 3) {
                                  throw Exception(name() + "::string_substr expects 3 arguments");
                              }
                              std::string str    = args[0];
                              int         start  = args[1];
                              int         length = args[2];
                              return str.substr(start, length);
                          });
    }
};

}  // namespace Modules

#endif  // MODULES_STRINGMODULE_HPP
