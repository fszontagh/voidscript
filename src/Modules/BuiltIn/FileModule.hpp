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
    FileModule() { setModuleName("File"); }

    void registerFunctions() override {
        std::vector<FunctParameterInfo> params = {
            { "file_name", Symbols::Variables::Type::STRING, "The file name" }
        };

        REGISTER_FUNCTION("file_get_contents", Symbols::Variables::Type::STRING, params, "Read the content of a file",
                          [](const FunctionArguments & args) -> Symbols::ValuePtr {
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
            { "file_name", Symbols::Variables::Type::STRING,  "The file name"                              },
            { "content",   Symbols::Variables::Type::STRING,  "The content to write to the file"           },
            { "overwrite", Symbols::Variables::Type::BOOLEAN, "Whether to overwrite the file if it exists" }
        };
        // Write content to file, with optional overwrite
        REGISTER_FUNCTION("file_put_contents", Symbols::Variables::Type::NULL_TYPE, params, "Write content into a file",
                          [](FunctionArguments & args) -> Symbols::ValuePtr {
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
            { "file_name", Symbols::Variables::Type::STRING, "The file name" }
        };

        // Check if file exists
        REGISTER_FUNCTION("file_exists", Symbols::Variables::Type::BOOLEAN, params, "Check if a file exists or not",
                          [](FunctionArguments & args) -> Symbols::ValuePtr {
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
            { "file_name", Symbols::Variables::Type::STRING, "The file name" }
        };

        REGISTER_FUNCTION("file_size", Symbols::Variables::Type::INTEGER, params, "Get the size of a file",
                          [](const FunctionArguments & args) -> Symbols::ValuePtr {
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
    }
};

}  // namespace Modules
#endif  // MODULES_FILEMODULE_HPP
