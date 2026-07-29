// EncodingModule.hpp
#ifndef MODULES_ENCODINGMODULE_HPP
#define MODULES_ENCODINGMODULE_HPP

#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

/**
 * @brief Text encoding helpers - the glue for HTTP/JSON/data work. No external dependency.
 *
 *   url_encode / url_decode      (application/x-www-form-urlencoded, RFC 3986 unreserved)
 *   hex_encode / hex_decode      (lower-case hex)
 *   html_escape / html_unescape  (& < > " ')
 *   ord(char) -> int             chr(int) -> string
 */
class EncodingModule : public BaseModule {
  public:
    EncodingModule() {
        setModuleName("Encoding");
        setDescription("URL, hex and HTML encoding/decoding helpers plus ord()/chr()");
        setBuiltIn(true);
    }

    void registerFunctions() override {
        using T = Symbols::Variables::Type;
        std::vector<Symbols::FunctionParameterInfo> s = { { "value", T::STRING, "The string" } };
        std::vector<Symbols::FunctionParameterInfo> i = { { "code", T::INTEGER, "A byte value 0-255" } };

        REGISTER_FUNCTION("url_encode", T::STRING, s, "Percent-encode a string for use in a URL/query",
                          Modules::EncodingModule::UrlEncode);
        REGISTER_FUNCTION("url_decode", T::STRING, s, "Decode a percent-encoded string (also '+' -> space)",
                          Modules::EncodingModule::UrlDecode);
        REGISTER_FUNCTION("hex_encode", T::STRING, s, "Encode a string as lower-case hexadecimal",
                          Modules::EncodingModule::HexEncode);
        REGISTER_FUNCTION("hex_decode", T::STRING, s, "Decode a hexadecimal string back to bytes",
                          Modules::EncodingModule::HexDecode);
        REGISTER_FUNCTION("html_escape", T::STRING, s, "Escape &, <, >, \" and ' for safe HTML output",
                          Modules::EncodingModule::HtmlEscape);
        REGISTER_FUNCTION("html_unescape", T::STRING, s, "Reverse html_escape (the five basic entities)",
                          Modules::EncodingModule::HtmlUnescape);
        REGISTER_FUNCTION("ord", T::INTEGER, s, "Byte value (0-255) of the first character",
                          Modules::EncodingModule::Ord);
        REGISTER_FUNCTION("chr", T::STRING, i, "One-character string for a byte value (0-255)",
                          Modules::EncodingModule::Chr);
    }

  private:
    static std::string strArg(Symbols::FunctionArguments & args, const char * fn) {
        if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::STRING) {
            throw std::runtime_error(std::string(fn) + " expects one string argument");
        }
        return args[0]->get<std::string>();
    }

    static Symbols::ValuePtr UrlEncode(Symbols::FunctionArguments & args) {
        const std::string in = strArg(args, "url_encode");
        static const char hex[] = "0123456789ABCDEF";
        std::string       out;
        for (unsigned char c : in) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                out.push_back(static_cast<char>(c));
            } else {
                out.push_back('%');
                out.push_back(hex[c >> 4]);
                out.push_back(hex[c & 0x0F]);
            }
        }
        return Symbols::ValuePtr(out);
    }

    static int hexVal(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    static Symbols::ValuePtr UrlDecode(Symbols::FunctionArguments & args) {
        const std::string in = strArg(args, "url_decode");
        std::string       out;
        for (size_t i = 0; i < in.size(); ++i) {
            if (in[i] == '%' && i + 2 < in.size()) {
                const int hi = hexVal(in[i + 1]);
                const int lo = hexVal(in[i + 2]);
                if (hi >= 0 && lo >= 0) {
                    out.push_back(static_cast<char>((hi << 4) | lo));
                    i += 2;
                    continue;
                }
            }
            out.push_back(in[i] == '+' ? ' ' : in[i]);
        }
        return Symbols::ValuePtr(out);
    }

    static Symbols::ValuePtr HexEncode(Symbols::FunctionArguments & args) {
        const std::string in = strArg(args, "hex_encode");
        static const char hex[] = "0123456789abcdef";
        std::string       out;
        out.reserve(in.size() * 2);
        for (unsigned char c : in) {
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 0x0F]);
        }
        return Symbols::ValuePtr(out);
    }

    static Symbols::ValuePtr HexDecode(Symbols::FunctionArguments & args) {
        const std::string in = strArg(args, "hex_decode");
        if (in.size() % 2 != 0) {
            throw std::runtime_error("hex_decode: input length must be even");
        }
        std::string out;
        out.reserve(in.size() / 2);
        for (size_t i = 0; i < in.size(); i += 2) {
            const int hi = hexVal(in[i]);
            const int lo = hexVal(in[i + 1]);
            if (hi < 0 || lo < 0) {
                throw std::runtime_error("hex_decode: invalid hex digit");
            }
            out.push_back(static_cast<char>((hi << 4) | lo));
        }
        return Symbols::ValuePtr(out);
    }

    static Symbols::ValuePtr HtmlEscape(Symbols::FunctionArguments & args) {
        const std::string in = strArg(args, "html_escape");
        std::string       out;
        for (char c : in) {
            switch (c) {
                case '&':  out += "&amp;";  break;
                case '<':  out += "&lt;";   break;
                case '>':  out += "&gt;";   break;
                case '"':  out += "&quot;"; break;
                case '\'': out += "&#39;";  break;
                default:   out.push_back(c);
            }
        }
        return Symbols::ValuePtr(out);
    }

    static Symbols::ValuePtr HtmlUnescape(Symbols::FunctionArguments & args) {
        const std::string in = strArg(args, "html_unescape");
        std::string       out;
        for (size_t i = 0; i < in.size();) {
            if (in[i] == '&') {
                if (in.compare(i, 5, "&amp;") == 0)  { out.push_back('&');  i += 5; continue; }
                if (in.compare(i, 4, "&lt;") == 0)   { out.push_back('<');  i += 4; continue; }
                if (in.compare(i, 4, "&gt;") == 0)   { out.push_back('>');  i += 4; continue; }
                if (in.compare(i, 6, "&quot;") == 0) { out.push_back('"');  i += 6; continue; }
                if (in.compare(i, 5, "&#39;") == 0)  { out.push_back('\''); i += 5; continue; }
            }
            out.push_back(in[i++]);
        }
        return Symbols::ValuePtr(out);
    }

    static Symbols::ValuePtr Ord(Symbols::FunctionArguments & args) {
        const std::string in = strArg(args, "ord");
        if (in.empty()) {
            throw std::runtime_error("ord: expects a non-empty string");
        }
        return Symbols::ValuePtr(static_cast<int>(static_cast<unsigned char>(in[0])));
    }

    static Symbols::ValuePtr Chr(Symbols::FunctionArguments & args) {
        if (args.size() != 1 || args[0]->getType() != Symbols::Variables::Type::INTEGER) {
            throw std::runtime_error("chr expects one integer argument");
        }
        const int code = args[0]->get<int>();
        if (code < 0 || code > 255) {
            throw std::runtime_error("chr: code must be 0-255");
        }
        return Symbols::ValuePtr(std::string(1, static_cast<char>(code)));
    }
};

}  // namespace Modules

#endif  // MODULES_ENCODINGMODULE_HPP
