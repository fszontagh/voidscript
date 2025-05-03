#ifndef VOIDSCRIPT_HPP
#define VOIDSCRIPT_HPP
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "Interpreter/Interpreter.hpp"
#include "Lexer/Lexer.hpp"
#include "Modules/BuiltIn/PrintModule.hpp"
#include "Modules/ModuleManager.hpp"
// Variable helper functions (typeof)
#include "Modules/BuiltIn/VariableHelpersModule.hpp"
// String helper functions
#include "Modules/BuiltIn/StringModule.hpp"
// Array helper functions (sizeof)
#include "Modules/BuiltIn/ArrayModule.hpp"
// File I/O
#include "Modules/BuiltIn/FileModule.hpp"
// JSON encode/decode
#include "Modules/BuiltIn/JsonModule.hpp"
#include "Modules/BuiltIn/ModuleHelperModule.hpp"
#ifdef FCGI
#    include "Modules/BuiltIn/HeaderModule.hpp"
#endif
#include "Interpreter/OperationsFactory.hpp"
#include "options.h"
#include "Parser/Parser.hpp"
#include "Symbols/Value.hpp"

class VoidScript {
  private:
    // Debug flags for various components
    bool                            debugLexer_       = false;
    bool                            debugParser_      = false;
    bool                            debugInterpreter_ = false;
    bool                            debugSymbolTable_ = false;
    std::vector<std::string>        files;
    // Only parse between open/close tags if enabled
    bool                            enableTags_          = false;
    // Suppress printing text outside tags when filtering is enabled
    bool                            suppressTagsOutside_ = false;
    // Script parameters passed after the script filename
    std::vector<std::string>        scriptArgs_;
    std::shared_ptr<Lexer::Lexer>   lexer  = nullptr;
    std::shared_ptr<Parser::Parser> parser = nullptr;

    static std::string readFile(const std::string & file) {
        // Read from stdin if '-' is specified
        if (file == "-") {
            return std::string(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
        }
        if (!std::filesystem::exists(file)) {
            throw std::runtime_error("File " + file + " does not exist");
        }
        std::ifstream input(file, std::ios::in);
        if (!input.is_open()) {
            throw std::runtime_error("Could not open file " + file);
        }
        std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        input.close();
        return content;
    }

  public:
    /**
     * @param file               initial script file
     * @param debugLexer         enable lexer debug output
     * @param debugParser        enable parser debug output
     * @param debugInterpreter   enable interpreter debug output
     */
    VoidScript(const std::string & file, bool debugLexer = false, bool debugParser = false,
               bool debugInterpreter = false, bool debugSymbolTable = false, bool enableTags = false,
               bool suppressTagsOutside = false, std::vector<std::string> scriptArgs = {}) :
        debugLexer_(debugLexer),
        debugParser_(debugParser),
        debugInterpreter_(debugInterpreter),
        debugSymbolTable_(debugSymbolTable),
        enableTags_(enableTags),
        suppressTagsOutside_(suppressTagsOutside),
        scriptArgs_(std::move(scriptArgs)),
        lexer(std::make_shared<Lexer::Lexer>()),
        parser(std::make_shared<Parser::Parser>()) {
        // Register built-in modules (print, etc.)
        // print functions
        Modules::ModuleManager::instance().addModule(std::make_unique<Modules::PrintModule>());
        // variable helpers (typeof)
        Modules::ModuleManager::instance().addModule(std::make_unique<Modules::VariableHelpersModule>());
        // string helper functions
        Modules::ModuleManager::instance().addModule(std::make_unique<Modules::StringModule>());
        // array helper functions (sizeof)
        Modules::ModuleManager::instance().addModule(std::make_unique<Modules::ArrayModule>());
        // file I/O builtin
        Modules::ModuleManager::instance().addModule(std::make_unique<Modules::FileModule>());
        // JSON encode/decode builtin
        Modules::ModuleManager::instance().addModule(std::make_unique<Modules::JsonModule>());
#ifdef FCGI
        // FastCGI header() function module
        Modules::ModuleManager::instance().addModule(std::make_unique<Modules::HeaderModule>());
#endif
        // Module helper builtin (list, exists, info for plugin modules)
        Modules::ModuleManager::instance().addModule(std::make_unique<Modules::ModuleHelperModule>());
        this->files.emplace(this->files.begin(), file);

        lexer->setKeyWords(Parser::Parser::keywords);
    }

    int run() {
        try {
            // Load plugin modules from 'modules' directory (case-insensitive)
            Modules::ModuleManager::instance().loadPlugins("Modules");
            Modules::ModuleManager::instance().loadPlugins(MODULES_FOLDER);
            // Register all built-in and plugin modules before execution
            Modules::ModuleManager::instance().registerAll();
            while (!files.empty()) {
                std::string       file         = files.back();
                const std::string file_content = readFile(file);
                files.pop_back();
                // Split content into segments: code inside tags and outside tags
                std::vector<std::pair<bool, std::string>> segments;
                if (!enableTags_) {
                    // Whole file is code to parse
                    segments.emplace_back(true, file_content);
                } else {
                    std::string openTag(PARSER_OPEN_TAG);
                    std::string closeTag(PARSER_CLOSE_TAG);
                    size_t      pos = 0;
                    while (pos < file_content.size()) {
                        size_t start = file_content.find(openTag, pos);
                        if (start == std::string::npos) {
                            // Remaining outside text
                            std::string outside = file_content.substr(pos);
                            if (!suppressTagsOutside_) {
                                segments.emplace_back(false, outside);
                            }
                            break;
                        }
                        // Outside text before tag
                        if (start > pos && !suppressTagsOutside_) {
                            segments.emplace_back(false, file_content.substr(pos, start - pos));
                        }
                        // Inside tag code
                        size_t      code_start = start + openTag.size();
                        size_t      end        = file_content.find(closeTag, code_start);
                        std::string code;
                        if (end != std::string::npos) {
                            code = file_content.substr(code_start, end - code_start);
                            pos  = end + closeTag.size();
                        } else {
                            // No closing tag: take until end
                            code = file_content.substr(code_start);
                            pos  = file_content.size();
                        }
                        segments.emplace_back(true, code);
                    }
                }

                std::string _default_namespace_ = file;
                std::replace(_default_namespace_.begin(), _default_namespace_.end(), '.', '_');

                Symbols::SymbolContainer::instance()->create(_default_namespace_);

                const std::string ns = Symbols::SymbolContainer::instance()->currentScopeName();
                // Pre-define script arguments: $argc (int) and $argv (string array as object map)
                {
                    using namespace Interpreter;
                    using namespace Symbols;
                    // Define argc (including the script name)
                    int argc_val = static_cast<int>(scriptArgs_.size()) + 1;
                    OperationsFactory::defineSimpleConstantVariable("argc", Value(argc_val), ns, file, 0, 0);
                    // Define argv as object map: argv[0] = script name, then parameters
                    Value::ObjectMap argv_map;
                    // Script filename at index 0
                    argv_map["0"] = Value(file);
                    // Subsequent entries for each script parameter
                    for (size_t i = 0; i < scriptArgs_.size(); ++i) {
                        argv_map[std::to_string(i + 1)] = Value(scriptArgs_[i]);
                    }
                    OperationsFactory::defineSimpleConstantVariable("argv", Value(argv_map), ns, file, 0, 0);
                }

                // Process each segment: either plain text or code to execute
                for (const auto & seg : segments) {
                    if (!seg.first) {
                        // Outside tag text: print as-is
                        std::cout << seg.second;
                    } else {
                        // Inside tag code: tokenize, parse, and execute
                        this->lexer->addNamespaceInput(ns, seg.second);
                        const auto tokens = this->lexer->tokenizeNamespace(ns);
                        if (debugLexer_) {
                            std::cerr << "[Debug][Lexer] Tokens for namespace '" << ns << "':\n";
                            for (const auto & tok : tokens) {
                                std::cerr << tok.dump();
                            }
                        }
                        parser->parseScript(tokens, file_content, file);
                        if (debugParser_) {
                            std::cerr << "[Debug][Parser] Operations for namespace '" << ns << "':\n";
                            for (const auto & op : Operations::Container::instance()->getAll(ns)) {
                                std::cerr << op->toString() << "\n";
                            }
                        }
                        Interpreter::Interpreter interpreter(debugInterpreter_);
                        interpreter.run();
                        // Clear operations after execution to avoid re-running
                        Operations::Container::instance()->clear(ns);
                        if (debugSymbolTable_) {
                            std::cout << Symbols::SymbolContainer::dump() << "\n";
                        }
                    }
                }
            }  // while (!files.empty())

            return 0;
        } catch (const std::exception & e) {
            std::cerr << e.what() << '\n';
            return 1;
        }
        return 1;
    }
};  // class VoidScript

#endif  // VOIDSCRIPT_HPP
