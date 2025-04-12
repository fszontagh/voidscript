#include <filesystem>
#include <fstream>

#include "Builtins/PrintModule.hpp"
#include "ScriptInterpreter.hpp"

static bool DEBUG = false;

int main(int argc, char * argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [-d / --debug] <script_file>\n";
        return 1;
    }

    std::string file;
    if (argc == 2) {
        file = argv[1];
    } else if (argc == 3 && (std::string(argv[1]) == "-d" || std::string(argv[1]) == "--debug")) {
        DEBUG = true;
        file  = argv[2];
    } else {
        std::cerr << "Usage: " << argv[0] << " [-d / --debug] <script_file>\n";
        return 1;
    }

    if (!std::filesystem::exists(file)) {
        std::cerr << "Error: File " << file << " does not exist.\n";
        return 1;
    }

    const std::string filename = std::filesystem::canonical(file).string();

    try {
        std::ifstream input(filename);
        if (!input.is_open()) {
            std::cerr << "Error: Could not open file " << filename << "\n";
            return 1;
        }

        std::string        content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        ScriptInterpreter interp;
        interp.registerFunction("print", std::make_shared<PrintFunction>());
        interp.executeScript(content, filename, DEBUG);
    } catch (const std::exception & e) {
        std::cerr << "Parser error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
