#ifndef VOIDSCRIPT_HPP
#define VOIDSCRIPT_HPP
#include <filesystem>
#include <fstream>
#include <string>

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
#include "Parser/Parser.hpp"

class VoidScript {
  private:
    // Debug flags for various components
    bool                            debugLexer_       = false;
    bool                            debugParser_      = false;
    bool                            debugInterpreter_ = false;
    bool                            debugSymbolTable_ = false;
    std::vector<std::string>        files;
    std::shared_ptr<Lexer::Lexer>   lexer  = nullptr;
    std::shared_ptr<Parser::Parser> parser = nullptr;

    static std::string readFile(const std::string & file) {
        if (!std::filesystem::exists(file)) {
            throw std::runtime_error("File " + file + " does not exits");
        }
        std::ifstream input(file, std::ios::in);
        if (!input.is_open()) {
            throw std::runtime_error("Could not open file " + file);
            return "";
        }
        std::string content = std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
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
               bool debugInterpreter = false, bool debugSymbolTable = false) :
        debugLexer_(debugLexer),
        debugParser_(debugParser),
        debugInterpreter_(debugInterpreter),
        debugSymbolTable_(debugSymbolTable),
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
        this->files.emplace(this->files.begin(), file);

        lexer->setKeyWords(Parser::Parser::keywords);
    }

    int run() {
        try {
            // Load plugin modules from 'modules' directory (case-insensitive)
            Modules::ModuleManager::instance().loadPlugins("modules");
            Modules::ModuleManager::instance().loadPlugins("Modules");
            // Register all built-in and plugin modules before execution
            Modules::ModuleManager::instance().registerAll();
            while (!files.empty()) {
                std::string       file         = files.back();
                const std::string file_content = readFile(file);
                files.pop_back();

                std::string _default_namespace_ = file;
                std::replace(_default_namespace_.begin(), _default_namespace_.end(), '.', '_');

                Symbols::SymbolContainer::instance()->create(_default_namespace_);

                const std::string ns = Symbols::SymbolContainer::instance()->currentScopeName();

                this->lexer->addNamespaceInput(ns, file_content);
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

                // Execute interpreter with optional debug output
                Interpreter::Interpreter interpreter(debugInterpreter_);
                interpreter.run();
                if (debugSymbolTable_) {
                    std::cout << Symbols::SymbolContainer::dump() << "\n";
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
