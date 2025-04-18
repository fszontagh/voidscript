#include <filesystem>
#include <iostream>
#include <unordered_map>

#include "options.h"
#include "VoidScript.hpp"

// Supported command-line parameters and descriptions
const std::unordered_map<std::string, std::string> params = {
    { "--help",    "Print this help message"                                                                     },
    { "--version", "Print the version of the program"                                                            },
    { "--debug",   "Enable debug output (all components or use --debug=lexer, parser, interpreter, symboltable)" },
};

int main(int argc, char * argv[]) {
    std::string usage = "Usage: " + std::string(argv[0]);
    for (const auto & [key, value] : params) {
        usage.append(" [" + key + "]");
    }
    // Parse arguments: allow --help, --version, --debug[=component], and a single file
    bool debugLexer       = false;
    bool debugParser      = false;
    bool debugInterp      = false;
    bool debugSymbolTable = false;

    std::string file;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help") {
            std::cout << usage << "\n";
            for (const auto & [key, value] : params) {
                std::cout << "  " << key << ": " << value << "\n";
            }
            return 0;
        } else if (a == "--version") {
            std::cout << "Version:      " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH << " ("
                      << VERSION_GIT_HASH << ")\n";
            std::cout << "Architecture: " << VERSION_ARCH << "\n";
            std::cout << "System:       " << VERSION_SYSTEM_NAME << "\n";
            return 0;
        } else if (a.rfind("--debug", 0) == 0) {
            if (a == "--debug") {
                debugLexer = debugParser = debugInterp = true;
            } else if (a.rfind("--debug=", 0) == 0) {
                std::string comp = a.substr(std::string("--debug=").size());
                if (comp == "lexer") {
                    debugLexer = true;
                } else if (comp == "parser") {
                    debugParser = true;
                } else if (comp == "interpreter") {
                    debugInterp = true;
                } else if (comp == "symboltable") {
                    debugSymbolTable = true;
                } else {
                    std::cerr << "Error: Unknown debug component '" << comp << "'\n";
                    std::cerr << usage << "\n";
                    return 1;
                }
            } else {
                std::cerr << "Error: Unknown option '" << a << "'\n";
                std::cerr << usage << "\n";
                return 1;
            }
        } else if (a.starts_with("-")) {
            std::cerr << "Error: Unknown option '" << a << "'\n";
            std::cerr << usage << "\n";
            return 1;
        } else if (file.empty()) {
            file = a;
        } else {
            std::cerr << "Error: Multiple files specified\n";
            std::cerr << usage << "\n";
            return 1;
        }
    }
    if (file.empty()) {
        std::cerr << "Error: No input file specified\n";
        std::cerr << usage << "\n";
        return 1;
    }

    if (!std::filesystem::exists(file)) {
        std::cerr << "Error: File " << file << " does not exist.\n";
        return 1;
    }

    const std::string filename = std::filesystem::canonical(file).string();

    // Initialize and run with debug options
    VoidScript voidscript(filename, debugLexer, debugParser, debugInterp, debugSymbolTable);
    return voidscript.run();
}
