#ifndef VALUE_HPP
#define VALUE_HPP
#include <cstdint>
#include <string>
#include <variant>

#include "VariableTypes.hpp"

class Value {
  public:
    Variables::Type          type = Variables::Type::VT_NULL;
    Variables::DataContainer data;

    Value() : type(Variables::Type::VT_NULL) {}

    Value(Variables::Type t, double val) : type(t), data(std::move(val)) {}

    Value(Variables::Type t, int val) : type(t), data(std::move(val)) {}

    Value(Variables::Type t, const std::string & val) : type(t), data(val) {}

    Value(Variables::Type t, bool val) : type(t), data(std::move(val)) {}

    static Value fromInt(int val) { return Value(Variables::Type::VT_INT, val); }

    static Value fromDouble(double val) { return Value(Variables::Type::VT_DOUBLE, val); }

    static Value fromString(const std::string & val) { return { Variables::Type::VT_STRING, val }; }

    static Value fromBoolean(bool val) { return { Variables::Type::VT_BOOLEAN, val }; }

    std::string ToString() const { return decodeEscapes(Variables::ToString(data, type)); }

    std::string TypeToString() const { return Variables::TypeToString(type); }

  private:
    Value(Variables::Type t, std::variant<int, double, std::string, bool> && val) : type(t), data(std::move(val)) {}

    static std::string decodeEscapes(const std::string & input) {
        std::string result;
        size_t      i = 0;

        auto hexToChar = [](const std::string & hex) -> char {
            return static_cast<char>(std::stoi(hex, nullptr, 16));
        };

        auto hexToUTF8 = [](uint32_t codepoint) -> std::string {
            std::string out;
            if (codepoint <= 0x7F) {
                out += static_cast<char>(codepoint);
            } else if (codepoint <= 0x7FF) {
                out += static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
                out += static_cast<char>(0x80 | (codepoint & 0x3F));
            } else if (codepoint <= 0xFFFF) {
                out += static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
                out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (codepoint & 0x3F));
            } else if (codepoint <= 0x10FFFF) {
                out += static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
                out += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
                out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (codepoint & 0x3F));
            }
            return out;
        };

        while (i < input.size()) {
            if (input[i] == '\\' && i + 1 < input.size()) {
                char esc = input[i + 1];

                // Standard short escape sequences
                switch (esc) {
                    case 'n':
                        result += '\n';
                        i += 2;
                        continue;
                    case 't':
                        result += '\t';
                        i += 2;
                        continue;
                    case 'r':
                        result += '\r';
                        i += 2;
                        continue;
                    case 'b':
                        result += '\b';
                        i += 2;
                        continue;
                    case 'f':
                        result += '\f';
                        i += 2;
                        continue;
                    case 'v':
                        result += '\v';
                        i += 2;
                        continue;
                    case 'a':
                        result += '\a';
                        i += 2;
                        continue;
                    case '\\':
                        result += '\\';
                        i += 2;
                        continue;
                    case '"':
                        result += '"';
                        i += 2;
                        continue;
                    case '\'':
                        result += '\'';
                        i += 2;
                        continue;
                    case '?':
                        result += '?';
                        i += 2;
                        continue;
                    case '0':
                        result += '\0';
                        i += 2;
                        continue;
                    case 'x':
                        {
                            // Hexadecimal escape: \xHH
                            size_t      j = i + 2;
                            std::string hex;
                            while (j < input.size() && std::isxdigit(input[j]) && hex.size() < 2) {
                                hex += input[j++];
                            }
                            if (!hex.empty()) {
                                result += hexToChar(hex);
                                i = j;
                                continue;
                            }
                        }
                        break;
                    case 'u':
                    case 'U':
                        {
                            // Unicode escape: \uHHHH or \UHHHHHHHH
                            size_t      expected_len = (esc == 'u') ? 4 : 8;
                            size_t      j            = i + 2;
                            std::string hex;
                            while (j < input.size() && std::isxdigit(input[j]) && hex.size() < expected_len) {
                                hex += input[j++];
                            }
                            if (hex.size() == expected_len) {
                                uint32_t codepoint = std::stoul(hex, nullptr, 16);
                                result += hexToUTF8(codepoint);
                                i = j;
                                continue;
                            }
                        }
                        break;
                }

                // Ha ide jutunk, ismeretlen vagy érvénytelen escape
                result += '\\';
                result += esc;
                i += 2;
            } else {
                result += input[i++];
            }
        }

        return result;
    }
};
#endif  // VALUE_HPP
