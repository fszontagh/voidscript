#ifndef VOIDSCRIPT_HPP
#define VOIDSCRIPT_HPP
#include <filesystem>
#include <fstream>
#include <string>

#include "Interpreter/Interpreter.hpp"
#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"

class VoidScript {
  private:
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
    VoidScript(const std::string & file) :

        lexer(std::make_shared<Lexer::Lexer>()),
        parser(std::make_shared<Parser::Parser>()) {
        this->files.emplace(this->files.begin(), file);

        lexer->setKeyWords(Parser::Parser::keywords);
    }

    int run() {
        try {
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

                parser->parseScript(tokens, file_content, file);

                Interpreter::Interpreter interpreter;
                interpreter.run();

                std::cout << Symbols::SymbolContainer::dump() << "\n";
            }  // while (!files.empty())

            return 0;
        } catch (const std::exception & e) {
            std::cerr  << e.what() << '\n';
            return 1;
        }
        return 1;
    }
};  // class VoidScript

#endif  // VOIDSCRIPT_HPP
