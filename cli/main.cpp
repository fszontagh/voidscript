#include <unistd.h>  // for isatty, STDIN_FILENO

#include <iostream>
#include <unordered_map>
#include <vector>

#include "options.h"
#include "utils.h"
#include "VoidScript.hpp"

// Supported command-line parameters and descriptions
const std::unordered_map<std::string, std::string> params = {
    { "--help",                  "Print this help message"                                                                     },
    { "--version",               "Print the version of the program"                                                            },
    { "--debug",                 "Enable debug output (all components or use --debug=lexer, parser, interpreter, symboltable)" },
    { "--enable-tags",           "Only parse tokens between PARSER_OPEN_TAG and PARSER_CLOSE_TAG when enabled"                 },
    { "--suppress-tags-outside",
     "Suppress text outside PARSER_OPEN_TAG/PARSER_CLOSE_TAG when tag filtering is enabled"                                    },
    { "-c, --command",           "Execute script string instead of reading from file"                                          },
};

int main(int argc, char * argv[]) {
    std::string usage = "Usage: " + std::string(argv[0]);
    for (const auto & [key, value] : params) {
        usage.append(" [" + key + "]");
    }
    // [file] is optional; if omitted, script is read from stdin
    // Or use -c "script" to execute script string directly
    usage.append(" [file | -c \"script\"]");
    // Parse arguments: allow --help, --version, --debug[=component], --gendocs, and a single file
    bool debugLexer       = false;
    bool debugParser      = false;
    bool debugInterp      = false;
    bool debugSymbolTable = false;

    std::string              file;
    std::string              scriptContent;      // For -c option
    bool                     isCommandMode       = false;  // Flag to indicate -c usage
    bool                     enableTags          = false;
    bool                     suppressTagsOutside = false;
    // Collect script parameters (arguments after script filename)
    std::vector<std::string> scriptArgs;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help") {
            std::cout << usage << "\n";
            for (const auto & [key, value] : params) {
                std::cout << "  " << key << ": " << value << "\n";
            }
            return 0;
        }
        if (a == "--version") {
            std::cout << "Version:      " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH << " ("
                      << VERSION_GIT_HASH << ")\n";
            std::cout << "Architecture: " << VERSION_ARCH << "\n";
            std::cout << "System:       " << VERSION_SYSTEM_NAME << "\n";
            return 0;
        }
        if (a.rfind("--debug", 0) == 0) {
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
        } else if (a == "--enable-tags") {
            enableTags = true;
        } else if (a == "--suppress-tags-outside") {
            suppressTagsOutside = true;
        } else if (a == "-c" || a == "--command") {
            // Next argument should be the script content
            if (i + 1 >= argc) {
                std::cerr << "Error: Option '" << a << "' requires a script argument\n";
                std::cerr << usage << "\n";
                return 1;
            }
            scriptContent = argv[++i];
            isCommandMode = true;
            file = "<command-line>";  // Virtual filename for command mode
        } else if (a == "-") {
            // Read script from stdin
            file = a;
        } else if (a.starts_with("-")) {
            std::cerr << "Error: Unknown option '" << a << "'\n";
            std::cerr << usage << "\n";
            return 1;
        } else if (file.empty() && !isCommandMode) {
            // First non-option argument is the script file (only if not in command mode)
            file = a;
        } else {
            // Remaining non-option arguments are script parameters
            scriptArgs.emplace_back(a);
        }
    }
    if (file.empty() && !isCommandMode) {
        // No input file specified: read script from stdin
        file = "-";
    }
    // If reading from stdin but stdin is a tty (no piped input), show usage
    if (file == "-" && isatty(STDIN_FILENO)) {
        std::cerr << usage << "\n";
        return 1;
    }

    // Determine if reading from a file, stdin, or executing command
    std::string filename;
    if (isCommandMode) {
        // Command mode: use virtual filename
        filename = file;
    } else if (file == "-") {
        // Read script from standard input
        filename = file;
    } else {
        if (!utils::exists(file)) {
            std::cerr << "Error: File " << file << " does not exist.\n";
            return 1;
        }
        filename = file;  // there was a canonical
    }

    VoidScript voidscript(filename, debugLexer, debugParser, debugInterp, debugSymbolTable, enableTags,
                          suppressTagsOutside, scriptArgs);
    
    // If in command mode, set the script content directly
    if (isCommandMode) {
        voidscript.setScriptContent(scriptContent);
    }
    
    return voidscript.run();
}
