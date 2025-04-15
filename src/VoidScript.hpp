#ifndef VOIDSCRIPT_HPP
#define VOIDSCRIPT_HPP

#include <filesystem>
#include <fstream>
#include <string>

#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"

class VoidScript {
  private:
    std::string                     file;
    std::shared_ptr<Lexer::Lexer>   lexer  = nullptr;
    std::shared_ptr<Parser::Parser> parser = nullptr;
    std::string                     file_content;

  public:
    VoidScript(const std::string & file) : file(file) {
        if (!std::filesystem::exists(file)) {
            throw std::runtime_error("File " + file + " does not exits");
        }
        lexer  = std::make_shared<Lexer::Lexer>();
        parser = std::make_shared<Parser::Parser>();

        lexer->setKeyWords(Parser::Parser::keywords);

        // read in the file
        std::ifstream input(file, std::ios::in);
        if (!input.is_open()) {
            throw std::runtime_error("Could not open file " + file);
        }
        file_content = std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        input.close();
    }

    int run() {
        try {
            this->lexer->addNamespaceInput(this->file, this->file_content);
            const auto tokens = this->lexer->tokenizeNamespace(this->file);

            std::cout << "--- Tokens ---" << '\n';
            for (const auto & token : tokens) {
                if (token.type != Lexer::Tokens::Type::END_OF_FILE) {
                    //token.print();
                }
            }
            std::cout << "--------------" << '\n';

            parser->parseProgram(tokens, this->file_content);
            const std::shared_ptr<Symbols::SymbolContainer> & symbol_container = parser->getSymbolContainer();

            std::cout << "\n--- Defined Variables ---" << '\n';
            for (const auto & symbol_ptr : symbol_container->listNamespace("variables")) {
                // Itt lehetne a szimbólumokat kiírni vagy tovább feldolgozni
                // Szükséges lehet dynamic_cast<> a konkrét típushoz (VariableSymbol)
                if (auto var_symbol = std::dynamic_pointer_cast<Symbols::VariableSymbol>(symbol_ptr)) {
                    std::cout << var_symbol->toString() << '\n';
                }
            }

            std::cout << "\n--- Defined Functions ---" << '\n';
            for (const auto & symbol_ptr : symbol_container->listNamespace("functions")) {
                if (auto func_symbol = std::dynamic_pointer_cast<Symbols::FunctionSymbol>(symbol_ptr)) {
                    std::cout << "Func Name: " << func_symbol->name()
                              << " return type: " << Symbols::Variables::TypeToString(func_symbol->returnType())
                              << '\n';
                    for (const auto & func_param : func_symbol->parameters()) {
                        std::cout << "  Param: " << func_param.name
                                  << " Type: " << Symbols::Variables::TypeToString(func_param.type) << '\n';
                    }
                    std::cout << "  Context name: " << func_symbol->context() << '\n';
                    std::cout << "  Plain body: " << func_symbol->plainBody() << '\n';
                }
            }

            return 0;
        } catch (const Parser::SyntaxError & e) {
            std::cerr << "Syntax Error during parsing: " << e.what() << '\n';
            return 1;
        } catch (const std::exception & e) {
            std::cerr << "An error occurred: " << e.what() << '\n';
            return 1;
        }
        return 1;
    }
};  // class VoidScript

#endif  // VOIDSCRIPT_HPP
