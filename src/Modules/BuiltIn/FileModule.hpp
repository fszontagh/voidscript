// FileModule.hpp
#ifndef MODULES_FILEMODULE_HPP
#define MODULES_FILEMODULE_HPP

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "utils.h"

namespace Modules {

/**
 * @brief Module providing simple file I/O functions:
 *  file_get_contents(filename) -> string content
 *  file_put_contents(filename, content, overwrite) -> undefined, throws on error
 *  file_exists(filename) -> bool
 */
class FileModule : public BaseModule {

  public:
    FileModule() {
        setModuleName("File");
        setDescription("Provides file system operations including reading, writing, file existence checks, directory management, and file size queries");
        setBuiltIn(true);
    }

    void registerFunctions() override {
        std::vector<Symbols::FunctionParameterInfo> params = {
            { "file_name", Symbols::Variables::Type::STRING, "The file name", false, false }
        };

        REGISTER_FUNCTION("file_get_contents", Symbols::Variables::Type::STRING, params, "Read the content of a file",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw std::runtime_error("file_get_contents expects 1 argument");
                              }
                              if (args[0]->getType() != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("file_get_contents expects string filename");
                              }
                              const std::string filename = args[0]->get<std::string>();
                              if (!utils::exists(filename)) {
                                  throw std::runtime_error("File does not exist: " + filename);
                              }
                              std::ifstream input(filename, std::ios::in | std::ios::binary);
                              if (!input.is_open()) {
                                  throw std::runtime_error("Could not open file: " + filename);
                              }
                              std::string content((std::istreambuf_iterator<char>(input)),
                                                  std::istreambuf_iterator<char>());
                              input.close();
                              return Symbols::ValuePtr(content);
                          });

        params = {
            { "file_name", Symbols::Variables::Type::STRING,  "The file name", false, false },
            { "content",   Symbols::Variables::Type::STRING,  "The content to write to the file", false, false },
            { "overwrite", Symbols::Variables::Type::BOOLEAN, "Whether to overwrite the file if it exists", false, false }
        };
        // Write content to file, with optional overwrite
        REGISTER_FUNCTION("file_put_contents", Symbols::Variables::Type::NULL_TYPE, params, "Write content into a file",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 3) {
                                  throw std::runtime_error("file_put_contents expects 3 arguments");
                              }
                              if (args[0]->getType() != Symbols::Variables::Type::STRING ||
                                  args[1]->getType() != Symbols::Variables::Type::STRING ||
                                  args[2]->getType() != Symbols::Variables::Type::BOOLEAN) {
                                  throw std::runtime_error("file_put_contents expects (string, string, bool)");
                              }
                              const std::string filename  = args[0]->get<std::string>();
                              const std::string content   = args[1]->get<std::string>();
                              const bool        overwrite = args[2]->get<bool>();
                              if (!overwrite && utils::exists(filename)) {
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
                              return Symbols::ValuePtr::null();
                          });
        params = {
            { "file_name", Symbols::Variables::Type::STRING, "The file name", false, false }
        };

        // Check if file exists
        REGISTER_FUNCTION("file_exists", Symbols::Variables::Type::BOOLEAN, params, "Check if a file exists or not",
                          [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw std::runtime_error("file_exists expects 1 argument");
                              }
                              if (args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("file_exists expects string filename");
                              }
                              const std::string filename = args[0];
                              return utils::exists(filename);
                          });

        params = {
            { "file_name", Symbols::Variables::Type::STRING, "The file name", false, false }
        };

        REGISTER_FUNCTION("file_size", Symbols::Variables::Type::INTEGER, params, "Get the size of a file",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw std::runtime_error("file_size expects 1 argument");
                              }
                              if (args[0] != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("file_get_contents expects string filename");
                              }
                              const std::string filename = args[0]->get<std::string>();
                              if (utils::exists(filename) == false) {
                                  throw std::runtime_error("file_size: file not found: " + filename);
                              }
                              if (utils::is_directory((filename))) {
                                  return 4096;
                              }
                              size_t size = utils::file_size(filename);
                              return static_cast<int>(size);
                          });

        // Create directory with optional recursive creation
        params = {
            { "dir_path", Symbols::Variables::Type::STRING, "The directory path", false, false },
            { "recursive", Symbols::Variables::Type::BOOLEAN, "Whether to create parent directories recursively", true, false }
        };

        REGISTER_FUNCTION("mkdir", Symbols::Variables::Type::BOOLEAN, params, "Create a directory",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() < 1 || args.size() > 2) {
                                  throw std::runtime_error("mkdir expects 1 or 2 arguments");
                              }
                              if (args[0]->getType() != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("mkdir expects string directory path");
                              }
                              
                              const std::string dir_path = args[0]->get<std::string>();
                              bool recursive = false;
                              
                              if (args.size() == 2) {
                                  if (args[1]->getType() != Symbols::Variables::Type::BOOLEAN) {
                                      throw std::runtime_error("mkdir second argument must be boolean (recursive)");
                                  }
                                  recursive = args[1]->get<bool>();
                              }
                              
                              bool success;
                              if (recursive) {
                                  success = utils::create_directories(dir_path);
                              } else {
                                  success = utils::create_directory(dir_path);
                              }
                              
                              return Symbols::ValuePtr(success);
                          });

        // Remove directory (only if empty)
        params = {
            { "dir_path", Symbols::Variables::Type::STRING, "The directory path", false, false }
        };

        REGISTER_FUNCTION("rmdir", Symbols::Variables::Type::BOOLEAN, params, "Remove an empty directory",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw std::runtime_error("rmdir expects 1 argument");
                              }
                              if (args[0]->getType() != Symbols::Variables::Type::STRING) {
                                  throw std::runtime_error("rmdir expects string directory path");
                              }
                              
                              const std::string dir_path = args[0]->get<std::string>();
                              bool success = utils::remove_directory(dir_path);
                              
                              return Symbols::ValuePtr(success);
                          });
    }
};

}  // namespace Modules
#endif  // MODULES_FILEMODULE_HPP
