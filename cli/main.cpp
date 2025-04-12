#include <filesystem>
#include <fstream>

#include "Builtins/PrintModule.hpp"
#include "SScriptInterpreter.hpp"

static bool DEBUG = false;

int main(int argc, char * argv[]) {
    SScriptInterpreter interp;
    interp.registerFunction("print", std::make_shared<PrintFunction>());


    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [-d / --debug] <script_file>" << std::endl;
        return 1;
    }
    if (argc > 2) {
        if (std::string(argv[1]) == "-d" || std::string(argv[1]) == "--debug") {
            DEBUG = true;
        } else {
            std::cerr << "Usage: " << argv[0] << " [-d / --debug] <script_file>" << std::endl;
            return 1;
        }
    }
    if (argc > 3) {
        std::cerr << "Error: Too many arguments." << std::endl;
        return 1;
    }

    if (!std::filesystem::exists(argv[2])) {
        std::cerr << "Error: File " << argv[2] << " does not exist." << std::endl;
        return 1;
    }
    // get the absolute path of the file
    const std::string filename = std::filesystem::canonical(argv[2]).string();

    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return 1;
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        interp.executeScript(content, filename, DEBUG);
    } catch (const std::exception & e) {
        std::cerr << "Parser error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
