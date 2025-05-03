// FileModule.hpp
#ifndef MODULES_FILEMODULE_HPP
#define MODULES_FILEMODULE_HPP

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Modules/ModuleManager.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief Module providing simple file I/O functions:
 *  file_get_contents(filename) -> string content
 *  file_put_contents(filename, content, overwrite) -> undefined, throws on error
 *  file_exists(filename) -> bool
 */
class FileModule : public BaseModule {
  public:
    void registerModule() override {
        auto & mgr = ModuleManager::instance();
        // Read entire file content
        mgr.registerFunction("file_get_contents", [](FuncionArguments & args) {
            using namespace Symbols;
            if (args.size() != 1) {
                throw std::runtime_error("file_get_contents expects 1 argument");
            }
            if (args[0].getType() != Variables::Type::STRING) {
                throw std::runtime_error("file_get_contents expects string filename");
            }
            const std::string filename = args[0].get<std::string>();
            if (!std::filesystem::exists(filename)) {
                throw std::runtime_error("File does not exist: " + filename);
            }
            std::ifstream input(filename, std::ios::in | std::ios::binary);
            if (!input.is_open()) {
                throw std::runtime_error("Could not open file: " + filename);
            }
            std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
            input.close();
            return Value(content);
        });
        // Write content to file, with optional overwrite
        mgr.registerFunction("file_put_contents", [](FuncionArguments & args) {
            using namespace Symbols;
            if (args.size() != 3) {
                throw std::runtime_error("file_put_contents expects 3 arguments");
            }
            if (args[0].getType() != Variables::Type::STRING || args[1].getType() != Variables::Type::STRING ||
                args[2].getType() != Variables::Type::BOOLEAN) {
                throw std::runtime_error("file_put_contents expects (string, string, bool)");
            }
            const std::string filename  = args[0].get<std::string>();
            const std::string content   = args[1].get<std::string>();
            const bool        overwrite = args[2].get<bool>();
            if (!overwrite && std::filesystem::exists(filename)) {
                throw std::runtime_error("File already exists: " + filename);
            }
            std::ofstream output(filename, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!output.is_open()) {
                throw std::runtime_error("Could not open file for writing: " + filename);
            }
            output << content;
            if (!output) {
                throw std::runtime_error("Failed to write to file: " + filename);
            }
            output.close();
            return Value();
        });
        // Check if file exists
        mgr.registerFunction("file_exists", [](FuncionArguments & args) -> Symbols::Value {
            using namespace Symbols;
            if (args.size() != 1) {
                throw std::runtime_error("file_exists expects 1 argument");
            }
            if (args[0].getType() != Variables::Type::STRING) {
                throw std::runtime_error("file_exists expects string filename");
            }
            const std::string filename = args[0].get<std::string>();
            bool              exists   = std::filesystem::exists(filename);
            return Value(exists);
        });

        mgr.registerFunction("file_size", [](FuncionArguments & args) -> Symbols::Value {
            if (args.size() != 1) {
                throw std::runtime_error("file_size expects 1 argument");
            }
            if (args[0].getType() != Symbols::Variables::Type::STRING) {
                throw std::runtime_error("file_size expects string filename");
            }
            const std::string filename = args[0].get<std::string>();
            if (std::filesystem::exists(filename) == false) {
                throw std::runtime_error("file_size: file not found: " + filename);
            }
            if (std::filesystem::is_directory((filename))) {
                return Symbols::Value(4096);
            }
            size_t size = std::filesystem::file_size(filename);
            return Symbols::Value(static_cast<int>(size));
        });
    }
};

}  // namespace Modules
#endif  // MODULES_FILEMODULE_HPP
