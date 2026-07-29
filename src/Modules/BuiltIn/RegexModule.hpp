// RegexModule.hpp
#ifndef MODULES_REGEXMODULE_HPP
#define MODULES_REGEXMODULE_HPP

#include <regex>
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
 * @brief Regular expressions (ECMAScript syntax) via std::regex. No external dependency.
 *
 *   regex_match(pattern, subject)              -> bool (matches anywhere)
 *   regex_search(pattern, subject)             -> [ whole, group1, group2, ... ] or null
 *   regex_replace(pattern, subject, repl)      -> string (replaces ALL; $1 back-refs)
 *   regex_split(pattern, subject)              -> [ parts ]
 */
class RegexModule : public BaseModule {
  public:
    RegexModule() {
        setModuleName("Regex");
        setDescription("Regular-expression matching, search with capture groups, replace and split (ECMAScript)");
        setBuiltIn(true);
    }

    void registerFunctions() override {
        using T = Symbols::Variables::Type;
        std::vector<Symbols::FunctionParameterInfo> ps2 = {
            { "pattern", T::STRING, "The regular expression" },
            { "subject", T::STRING, "The string to test" }
        };
        std::vector<Symbols::FunctionParameterInfo> ps3 = {
            { "pattern", T::STRING, "The regular expression" },
            { "subject", T::STRING, "The string to operate on" },
            { "replacement", T::STRING, "Replacement text ($1, $2 back-references)" }
        };
        REGISTER_FUNCTION("regex_match", T::BOOLEAN, ps2, "Whether the pattern matches anywhere in the subject",
                          Modules::RegexModule::MatchFn);
        REGISTER_FUNCTION("regex_search", T::OBJECT, ps2,
                          "First match as [whole, group1, ...], or null if there is no match",
                          Modules::RegexModule::SearchFn);
        REGISTER_FUNCTION("regex_replace", T::STRING, ps3, "Replace all matches; supports $1 back-references",
                          Modules::RegexModule::ReplaceFn);
        REGISTER_FUNCTION("regex_split", T::OBJECT, ps2, "Split the subject on every match of the pattern",
                          Modules::RegexModule::SplitFn);
    }

  private:
    static std::regex compile(const Symbols::ValuePtr & pat, const char * fn) {
        if (pat->getType() != Symbols::Variables::Type::STRING) {
            throw std::runtime_error(std::string(fn) + ": pattern must be a string");
        }
        try {
            return std::regex(pat->get<std::string>());
        } catch (const std::regex_error & e) {
            throw std::runtime_error(std::string(fn) + ": invalid regex - " + e.what());
        }
    }

    static std::string subjectOf(const Symbols::ValuePtr & s, const char * fn) {
        if (s->getType() != Symbols::Variables::Type::STRING) {
            throw std::runtime_error(std::string(fn) + ": subject must be a string");
        }
        return s->get<std::string>();
    }

    static Symbols::ValuePtr MatchFn(Symbols::FunctionArguments & args) {
        if (args.size() != 2) {
            throw std::runtime_error("regex_match expects (string pattern, string subject)");
        }
        const std::regex  re      = compile(args[0], "regex_match");
        const std::string subject = subjectOf(args[1], "regex_match");
        return Symbols::ValuePtr(std::regex_search(subject, re));
    }

    static Symbols::ValuePtr SearchFn(Symbols::FunctionArguments & args) {
        if (args.size() != 2) {
            throw std::runtime_error("regex_search expects (string pattern, string subject)");
        }
        const std::regex  re      = compile(args[0], "regex_search");
        const std::string subject = subjectOf(args[1], "regex_search");
        std::smatch       m;
        if (!std::regex_search(subject, m, re)) {
            return Symbols::ValuePtr::null();
        }
        Symbols::ObjectMap out;
        for (size_t i = 0; i < m.size(); ++i) {
            out[std::to_string(i)] = Symbols::ValuePtr(m[i].str());
        }
        return Symbols::ValuePtr(out);
    }

    static Symbols::ValuePtr ReplaceFn(Symbols::FunctionArguments & args) {
        if (args.size() != 3 || args[2]->getType() != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("regex_replace expects (string pattern, string subject, string replacement)");
        }
        const std::regex  re      = compile(args[0], "regex_replace");
        const std::string subject = subjectOf(args[1], "regex_replace");
        const std::string repl    = args[2]->get<std::string>();
        return Symbols::ValuePtr(std::regex_replace(subject, re, repl));
    }

    static Symbols::ValuePtr SplitFn(Symbols::FunctionArguments & args) {
        if (args.size() != 2) {
            throw std::runtime_error("regex_split expects (string pattern, string subject)");
        }
        const std::regex  re      = compile(args[0], "regex_split");
        const std::string subject = subjectOf(args[1], "regex_split");
        // -1 selects the text between matches (the split parts).
        std::sregex_token_iterator it(subject.begin(), subject.end(), re, -1);
        std::sregex_token_iterator end;
        Symbols::ObjectMap         out;
        size_t                     i = 0;
        for (; it != end; ++it) {
            out[std::to_string(i++)] = Symbols::ValuePtr(it->str());
        }
        return Symbols::ValuePtr(out);
    }
};

}  // namespace Modules

#endif  // MODULES_REGEXMODULE_HPP
