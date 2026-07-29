// CsvModule.hpp
#ifndef MODULES_CSVMODULE_HPP
#define MODULES_CSVMODULE_HPP

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
 * @brief CSV parse/encode with RFC 4180 quoting. No external dependency.
 *
 *   csv_parse(text [, delimiter])  -> [ [field, ...], ... ]   (array of row arrays)
 *   csv_encode(rows [, delimiter]) -> string
 *
 * Fields containing the delimiter, a double quote or a newline are wrapped in double
 * quotes on encode, with embedded quotes doubled; parse reverses that.
 */
class CsvModule : public BaseModule {
  public:
    CsvModule() {
        setModuleName("Csv");
        setDescription("Parse and encode CSV text with RFC 4180 quoting");
        setBuiltIn(true);
    }

    void registerFunctions() override {
        using T = Symbols::Variables::Type;
        std::vector<Symbols::FunctionParameterInfo> parse_params = {
            { "text", T::STRING, "CSV text to parse" },
            { "delimiter", T::STRING, "Field delimiter (default \",\")", true }
        };
        std::vector<Symbols::FunctionParameterInfo> encode_params = {
            { "rows", T::OBJECT, "Array of row arrays" },
            { "delimiter", T::STRING, "Field delimiter (default \",\")", true }
        };
        REGISTER_FUNCTION("csv_parse", T::OBJECT, parse_params,
                          "Parse CSV text into an array of row arrays (RFC 4180 quoting)",
                          Modules::CsvModule::Parse);
        REGISTER_FUNCTION("csv_encode", T::STRING, encode_params,
                          "Encode an array of row arrays into CSV text (RFC 4180 quoting)",
                          Modules::CsvModule::Encode);
    }

  private:
    static char delimiterOf(Symbols::FunctionArguments & args, const char * fn) {
        if (args.size() >= 2) {
            if (args[1]->getType() != Symbols::Variables::Type::STRING) {
                throw std::runtime_error(std::string(fn) + ": delimiter must be a string");
            }
            const std::string d = args[1]->get<std::string>();
            if (d.size() != 1) {
                throw std::runtime_error(std::string(fn) + ": delimiter must be a single character");
            }
            return d[0];
        }
        return ',';
    }

    static Symbols::ValuePtr Parse(Symbols::FunctionArguments & args) {
        if (args.empty() || args[0]->getType() != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("csv_parse expects (string text [, string delimiter])");
        }
        const std::string in    = args[0]->get<std::string>();
        const char        delim = delimiterOf(args, "csv_parse");

        std::vector<std::vector<std::string>> rows;
        std::vector<std::string>              row;
        std::string                           field;
        bool                                  inQuotes = false;
        bool                                  sawAny   = false;  // did this line have any content?

        const auto endField = [&]() { row.push_back(field); field.clear(); };
        const auto endRow   = [&]() {
            endField();
            rows.push_back(row);
            row.clear();
            sawAny = false;
        };

        for (size_t i = 0; i < in.size(); ++i) {
            const char c = in[i];
            if (inQuotes) {
                if (c == '"') {
                    if (i + 1 < in.size() && in[i + 1] == '"') {  // escaped quote
                        field.push_back('"');
                        ++i;
                    } else {
                        inQuotes = false;
                    }
                } else {
                    field.push_back(c);
                }
                continue;
            }
            if (c == '"') {
                inQuotes = true;
                sawAny   = true;
            } else if (c == delim) {
                endField();
                sawAny = true;
            } else if (c == '\n') {
                endRow();
            } else if (c == '\r') {
                // swallow; the \n (if any) ends the row
            } else {
                field.push_back(c);
                sawAny = true;
            }
        }
        // Trailing field/row unless the input ended exactly on a newline with nothing after.
        if (sawAny || !field.empty() || !row.empty()) {
            endRow();
        }

        Symbols::ObjectMap outRows;
        for (size_t r = 0; r < rows.size(); ++r) {
            Symbols::ObjectMap cols;
            for (size_t c = 0; c < rows[r].size(); ++c) {
                cols[std::to_string(c)] = Symbols::ValuePtr(rows[r][c]);
            }
            outRows[std::to_string(r)] = Symbols::ValuePtr(cols);
        }
        return Symbols::ValuePtr(outRows);
    }

    // Read an array (ObjectMap) back in index order.
    static std::vector<Symbols::ValuePtr> indexed(const Symbols::ValuePtr & v) {
        std::vector<Symbols::ValuePtr> out;
        if (v->getType() != Symbols::Variables::Type::OBJECT && v->getType() != Symbols::Variables::Type::CLASS) {
            return out;
        }
        const auto & map = v->get<Symbols::ObjectMap>();
        for (size_t i = 0;; ++i) {
            auto it = map.find(std::to_string(i));
            if (it == map.end()) {
                break;
            }
            out.push_back(it->second);
        }
        return out;
    }

    static std::string quoteField(const std::string & s, char delim) {
        bool needs = s.find(delim) != std::string::npos || s.find('"') != std::string::npos ||
                     s.find('\n') != std::string::npos || s.find('\r') != std::string::npos;
        if (!needs) {
            return s;
        }
        std::string out = "\"";
        for (char c : s) {
            if (c == '"') {
                out += "\"\"";
            } else {
                out.push_back(c);
            }
        }
        out.push_back('"');
        return out;
    }

    static Symbols::ValuePtr Encode(Symbols::FunctionArguments & args) {
        if (args.empty() || (args[0]->getType() != Symbols::Variables::Type::OBJECT &&
                             args[0]->getType() != Symbols::Variables::Type::CLASS)) {
            throw std::runtime_error("csv_encode expects (array rows [, string delimiter])");
        }
        const char  delim = delimiterOf(args, "csv_encode");
        std::string out;
        for (auto & rowV : indexed(args[0])) {
            std::vector<Symbols::ValuePtr> cols = indexed(rowV);
            for (size_t c = 0; c < cols.size(); ++c) {
                if (c) {
                    out.push_back(delim);
                }
                out += quoteField(cols[c]->toString(), delim);
            }
            out.push_back('\n');
        }
        return Symbols::ValuePtr(out);
    }
};

}  // namespace Modules

#endif  // MODULES_CSVMODULE_HPP
