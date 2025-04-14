#include <filesystem>
#include <fstream>

#include "Builtins/PrintModule.hpp"
#include "Builtins/SleepModule.hpp"
#include "ScriptInterpreter.hpp"

const std::unordered_map<std::string, std::string> params = {
    { "--help",    "Print this help message"          },
    { "--version", "Print the version of the program" },
};

int main(int argc, char * argv[]) {
    std::string usage = "Usage: " + std::string(argv[0]);
    for (const auto & [key, value] : params) {
        usage.append(" [" + key + "]");
    }
    if (argc < 2) {
        std::cerr << usage << "\n";
        return 1;
    }

    std::string file;

    const std::string arg = std::string(argv[1]);
    if (arg.starts_with("-")) {
        auto it = params.find(arg);
        if (it != params.end()) {
            if (arg == "--help") {
                std::cout << usage << "\n";
                for (const auto & [key, value] : params) {
                    std::cout << "  " << key << ": " << value << "\n";
                }
                return 0;
            }
            if (arg == "--version") {
                std::cout << "Version:      " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH;
                std::cout << " (" << VERSION_GIT_HASH << ")\n";
                std::cout << "Architecture: " << VERSION_ARCH << "\n";
                std::cout << "System:       " << VERSION_SYSTEM_NAME << "\n";
                return 0;
            }
            return 0;
        }
        std::cerr << "Error: Unknown option " << arg << "\n";
        std::cerr << usage << "\n";
        return 1;
    }
    file = arg;

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

        std::string       content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        ScriptInterpreter interp;
        interp.registerModule("print", std::make_shared<PrintFunction>());
        interp.registerModule("sleep", std::make_shared<SleepFunction>());
        interp.executeScript(content, filename, "_default_", false);
    } catch (const std::exception & e) {
        std::cerr << "Parser error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
