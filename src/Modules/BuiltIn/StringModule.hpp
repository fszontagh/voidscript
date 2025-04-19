// StringModule.hpp
#ifndef MODULES_STRINGMODULE_HPP
#define MODULES_STRINGMODULE_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include "Modules/BaseModule.hpp"
#include "Modules/ModuleManager.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

/**
 * @brief Module providing string helper functions.
 * Functions:
 *   string_length(string $in) -> length of the string
 *   string_replace(string $in, string $from, string $to, bool $replace_all)
 *   string_substr(string $in, int from, int length) -> substring
 */
class StringModule : public BaseModule {
 public:
    void registerModule() override {
        auto &mgr = ModuleManager::instance();
        // string_length
        mgr.registerFunction("string_length", [](const std::vector<Symbols::Value> &args) {
            using namespace Symbols;
            if (args.size() != 1) {
                throw std::runtime_error("string_length expects exactly one argument");
            }
            if (args[0].getType() != Variables::Type::STRING) {
                throw std::runtime_error("string_length expects a string argument");
            }
            const std::string &s = args[0].get<std::string>();
            return Value(static_cast<int>(s.size()));
        });
        // string_replace
        mgr.registerFunction("string_replace", [](const std::vector<Symbols::Value> &args) {
            using namespace Symbols;
            if (args.size() != 4) {
                throw std::runtime_error("string_replace expects 4 arguments");
            }
            if (args[0].getType() != Variables::Type::STRING ||
                args[1].getType() != Variables::Type::STRING ||
                args[2].getType() != Variables::Type::STRING ||
                args[3].getType() != Variables::Type::BOOLEAN) {
                throw std::runtime_error("string_replace argument types must be (string, string, string, boolean)");
            }
            std::string in = args[0].get<std::string>();
            const std::string &from = args[1].get<std::string>();
            const std::string &to = args[2].get<std::string>();
            bool replace_all = args[3].get<bool>();
            if (from.empty()) {
                throw std::runtime_error("string_replace: 'from' cannot be empty");
            }
            size_t pos = 0;
            if (replace_all) {
                while ((pos = in.find(from, pos)) != std::string::npos) {
                    in.replace(pos, from.length(), to);
                    pos += to.length();
                }
            } else {
                pos = in.find(from);
                if (pos != std::string::npos) {
                    in.replace(pos, from.length(), to);
                }
            }
            return Value(in);
        });
        // string_substr
        mgr.registerFunction("string_substr", [](const std::vector<Symbols::Value> &args) {
            using namespace Symbols;
            if (args.size() != 3) {
                throw std::runtime_error("string_substr expects 3 arguments");
            }
            if (args[0].getType() != Variables::Type::STRING ||
                args[1].getType() != Variables::Type::INTEGER ||
                args[2].getType() != Variables::Type::INTEGER) {
                throw std::runtime_error("string_substr argument types must be (string, int, int)");
            }
            const std::string &s = args[0].get<std::string>();
            int from = args[1].get<int>();
            int length = args[2].get<int>();
            if (from < 0 || length < 0) {
                throw std::runtime_error("string_substr: 'from' and 'length' must be non-negative");
            }
            size_t pos = static_cast<size_t>(from);
            if (pos > s.size()) {
                throw std::runtime_error("string_substr: 'from' out of range");
            }
            size_t len = static_cast<size_t>(length);
            std::string sub = s.substr(pos, len);
            return Value(sub);
        });
    }
};

} // namespace Modules

#endif // MODULES_STRINGMODULE_HPP