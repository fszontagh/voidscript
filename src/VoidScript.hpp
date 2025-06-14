#ifndef VOIDSCRIPT_HPP
#define VOIDSCRIPT_HPP
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>
#include <unistd.h>  // for readlink

#include "Interpreter/Interpreter.hpp"
#include "Lexer/Lexer.hpp"
#include "Modules/BuiltIn/ArrayModule.hpp"
#include "Modules/BuiltIn/ConversionModule.hpp"
#include "Modules/BuiltIn/FileModule.hpp"
#include "Modules/BuiltIn/JsonModule.hpp"
#include "Modules/BuiltIn/ModuleHelperModule.hpp"
#include "Modules/BuiltIn/PrintModule.hpp"
#include "Modules/BuiltIn/StringModule.hpp"
#include "Modules/BuiltIn/VariableHelpersModule.hpp"
#include "options.h"
#include "utils.h"
#ifndef _WIN32
#include <dlfcn.h>
#else
#include <windows.h>
#endif
#include <filesystem>
#ifdef FCGI
#    include "Modules/BuiltIn/HeaderModule.hpp"
#endif
#include "Interpreter/OperationsFactory.hpp"
#include "Parser/Parser.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

class VoidScript {
  private:
    // Debug flags for various components
    bool                            debugLexer_       = false;
    bool                            debugParser_      = false;
    bool                            debugInterpreter_ = false;
    bool                            debugSymbolTable_ = false;
    std::vector<std::string>        files;
    // Only parse between open/close tags if enabled
    bool                            enableTags_          = false;
    // Suppress printing text outside tags when filtering is enabled
    bool                            suppressTagsOutside_ = false;
    // Script parameters passed after the script filename
    std::vector<std::string>        scriptArgs_;
    // Direct script content for command mode (-c option)
    std::string                     directScriptContent_;
    bool                            hasDirectContent_ = false;
    std::shared_ptr<Lexer::Lexer>   lexer  = nullptr;
    std::shared_ptr<Parser::Parser> parser = nullptr;

    std::string readFile(const std::string & file) {
        // If we have direct script content (command mode), return it
        if (hasDirectContent_) {
            return directScriptContent_;
        }
        // Read from stdin if '-' is specified
        if (file == "-") {
            return std::string(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
        }
        if (!utils::exists(file)) {
            throw std::runtime_error("File " + file + " does not exist");
        }
        std::ifstream input(file, std::ios::in);
        if (!input.is_open()) {
            throw std::runtime_error("Could not open file " + file);
        }
        std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        input.close();
        return content;
    }

    // Load dynamic plugins from a directory
    void loadPlugins(const std::string & directory) {
        if (!utils::exists(directory) || !utils::is_directory(directory)) {
            return;
        }
        
        for (const auto & entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            
#ifndef _WIN32
            if (entry.path().extension() != ".so") {
                continue;
            }
#else
            if (entry.path().extension() != ".dll") {
                continue;
            }
#endif
            
            loadPlugin(entry.path().string());
        }
    }
    
    // Load a single plugin library
    void loadPlugin(const std::string & path) {
        void * handle;
        
#ifndef _WIN32
        handle = dlopen(path.c_str(), RTLD_NOW);
        if (!handle) {
            std::cerr << "Warning: Failed to load plugin " << path << ": " << dlerror() << std::endl;
            return;
        }

        using PluginInitFunc = void (*)();
        dlerror();  // Clear any existing errors
        PluginInitFunc init = reinterpret_cast<PluginInitFunc>(dlsym(handle, "plugin_init"));
        const char * dlsym_error = dlerror();
        if (dlsym_error) {
            std::cerr << "Warning: Plugin missing 'plugin_init' symbol: " << path << ": " << dlsym_error << std::endl;
            dlclose(handle);
            return;
        }
        
        // Call the plugin initialization function with error handling
        try {
            init();
        } catch (const std::exception & e) {
            std::cerr << "Warning: Plugin initialization failed for " << path << ": " << e.what() << std::endl;
        }
#else
        handle = LoadLibraryA(path.c_str());
        if (!handle) {
            std::cerr << "Warning: Failed to load plugin: " << path << std::endl;
            return;
        }

        using PluginInitFunc = void (*)();
        PluginInitFunc init = reinterpret_cast<PluginInitFunc>(GetProcAddress((HMODULE) handle, "plugin_init"));
        if (!init) {
            std::cerr << "Warning: Plugin missing 'plugin_init' symbol: " << path << std::endl;
            FreeLibrary((HMODULE) handle);
            return;
        }
        
        // Call the plugin initialization function
        init();
#endif
        // Note: We're not storing the handle for cleanup since modules register globally
        // In a production system, you might want to track loaded plugins for cleanup
    }

  public:
    /**
     * @param file               initial script file
     * @param debugLexer         enable lexer debug output
     * @param debugParser        enable parser debug output
     * @param debugInterpreter   enable interpreter debug output
     */
    VoidScript(const std::string & file, bool debugLexer = false, bool debugParser = false,
               bool debugInterpreter = false, bool debugSymbolTable = false, bool enableTags = false,
               bool suppressTagsOutside = false, std::vector<std::string> scriptArgs = {}) :
        debugLexer_(debugLexer),
        debugParser_(debugParser),
        debugInterpreter_(debugInterpreter),
        debugSymbolTable_(debugSymbolTable),
        enableTags_(enableTags),
        suppressTagsOutside_(suppressTagsOutside),
        scriptArgs_(std::move(scriptArgs)),
        lexer(std::make_shared<Lexer::Lexer>()),
        parser(std::make_shared<Parser::Parser>()) {
        // Initialize SymbolContainer with the main script file path
        // Assuming 'file' parameter is the absolute path desired for the scope name.
        Symbols::SymbolContainer::initialize(file);

        // Register built-in modules (print, etc.)
        auto symbolContainer = Symbols::SymbolContainer::instance();
        
        // print functions
        auto printModule = std::make_unique<Modules::PrintModule>();
        printModule->setModuleName("Print");
        symbolContainer->registerModule(Modules::make_base_module_ptr(std::move(printModule)));
        
        // variable helpers (typeof)
        auto varHelpersModule = std::make_unique<Modules::VariableHelpersModule>();
        varHelpersModule->setModuleName("VariableHelpers");
        symbolContainer->registerModule(Modules::make_base_module_ptr(std::move(varHelpersModule)));
        
        // string helper functions
        auto stringModule = std::make_unique<Modules::StringModule>();
        stringModule->setModuleName("String");
        symbolContainer->registerModule(Modules::make_base_module_ptr(std::move(stringModule)));
        
        // conversion functions (string_to_number, number_to_string)
        auto conversionModule = std::make_unique<Modules::ConversionModule>();
        conversionModule->setModuleName("Conversion");
        symbolContainer->registerModule(Modules::make_base_module_ptr(std::move(conversionModule)));
        
        // array helper functions (sizeof)
        auto arrayModule = std::make_unique<Modules::ArrayModule>();
        arrayModule->setModuleName("Array");
        symbolContainer->registerModule(Modules::make_base_module_ptr(std::move(arrayModule)));
        
        // file I/O builtin
        auto fileModule = std::make_unique<Modules::FileModule>();
        fileModule->setModuleName("File");
        symbolContainer->registerModule(Modules::make_base_module_ptr(std::move(fileModule)));
        
        // JSON encode/decode builtin
        auto jsonModule = std::make_unique<Modules::JsonModule>();
        jsonModule->setModuleName("Json");
        symbolContainer->registerModule(Modules::make_base_module_ptr(std::move(jsonModule)));
        
        // module helper functions (module_list, module_info, etc.)
        auto moduleHelperModule = std::make_unique<Modules::ModuleHelperModule>();
        moduleHelperModule->setModuleName("ModuleHelper");
        symbolContainer->registerModule(Modules::make_base_module_ptr(std::move(moduleHelperModule)));
        
#ifdef FCGI
        // FastCGI header() function module
        auto headerModule = std::make_unique<Modules::HeaderModule>();
        headerModule->setModuleName("Header");
        symbolContainer->registerModule(Modules::make_base_module_ptr(std::move(headerModule)));
#endif

        // Load dynamic plugins from modules directory
        // Try installed location first, then fall back to development location
        std::string modulesPath = MODULES_FOLDER;
        if (!utils::exists(modulesPath) || !utils::is_directory(modulesPath)) {
            // Fall back to development location relative to binary
            // Get the current executable path and assume modules are in the same directory
            char exePath[1024];
            ssize_t count = readlink("/proc/self/exe", exePath, sizeof(exePath));
            if (count != -1) {
                exePath[count] = '\0';
                std::string binPath(exePath);
                size_t lastSlash = binPath.find_last_of("/\\");
                std::string binDir = (lastSlash != std::string::npos) ? binPath.substr(0, lastSlash) : ".";
                modulesPath = binDir + "/Modules";
            } else {
                // Fallback if readlink fails - assume we're in build directory
                modulesPath = "./Modules";
            }
        }
        
        if (utils::exists(modulesPath) && utils::is_directory(modulesPath)) {
            loadPlugins(modulesPath);
        } else {
            std::cerr << "Warning: modules directory not found: " << modulesPath << std::endl;
        }

        this->files.emplace(this->files.begin(), file);

        lexer->setKeyWords(Parser::Parser::keywords);
    }

    /**
     * Set script content directly (for command mode -c option)
     * @param content The script content to execute
     */
    void setScriptContent(const std::string & content) {
        directScriptContent_ = content;
        hasDirectContent_ = true;
    }

    int run() {
        try {
            // Plugin loading is now handled directly by the modules themselves
            // Each module registers its functions with SymbolContainer
            while (!files.empty()) {
                std::string       file         = files.back();
                const std::string file_content = readFile(file);
                files.pop_back();
                // Split content into segments: code inside tags and outside tags
                std::vector<std::pair<bool, std::string>> segments;
                if (!enableTags_) {
                    // Whole file is code to parse
                    segments.emplace_back(true, file_content);
                } else {
                    std::string openTag(PARSER_OPEN_TAG);
                    std::string closeTag(PARSER_CLOSE_TAG);
                    size_t      pos = 0;
                    while (pos < file_content.size()) {
                        size_t start = file_content.find(openTag, pos);
                        if (start == std::string::npos) {
                            // Remaining outside text
                            std::string outside = file_content.substr(pos);
                            if (!suppressTagsOutside_) {
                                segments.emplace_back(false, outside);
                            }
                            break;
                        }
                        // Outside text before tag
                        if (start > pos && !suppressTagsOutside_) {
                            segments.emplace_back(false, file_content.substr(pos, start - pos));
                        }
                        // Inside tag code
                        size_t      code_start = start + openTag.size();
                        size_t      end        = file_content.find(closeTag, code_start);
                        std::string code;
                        if (end != std::string::npos) {
                            code = file_content.substr(code_start, end - code_start);
                            pos  = end + closeTag.size();
                        } else {
                            // No closing tag: take until end
                            code = file_content.substr(code_start);
                            pos  = file_content.size();
                        }
                        segments.emplace_back(true, code);
                    }
                }

                const std::string & current_file_scope_name = file;
                Symbols::SymbolContainer::instance()->create(current_file_scope_name);

                const std::string ns = Symbols::SymbolContainer::instance()->currentScopeName();
                // Pre-define script arguments: $argc (int) and $argv (string array as object map)
                {
                    // Define argc (including the script name)
                    int argc_val = static_cast<int>(scriptArgs_.size()) + 1;
                    Interpreter::OperationsFactory::defineSimpleConstantVariable("argc", argc_val, ns, file, 0, 0);
                    // Define argv as object map: argv[0] = script name, then parameters
                    Symbols::ObjectMap argv_map;
                    // Script filename at index 0
                    argv_map["0"] = file;
                    // Subsequent entries for each script parameter
                    for (size_t i = 0; i < scriptArgs_.size(); ++i) {
                        argv_map[std::to_string(i + 1)] = scriptArgs_[i];
                    }
                    Interpreter::OperationsFactory::defineSimpleConstantVariable("argv", argv_map, ns, file, 0, 0);
                }

                // Process each segment: either plain text or code to execute
                for (const auto & seg : segments) {
                    if (!seg.first) {
                        // Outside tag text: print as-is
                        std::cout << seg.second;
                    } else {
                        // Inside tag code: tokenize, parse, and execute
                        this->lexer->addNamespaceInput(ns, seg.second);
                        const auto tokens = this->lexer->tokenizeNamespace(ns);
                        if (debugLexer_) {
                            std::cerr << "[Debug][Lexer] Tokens for namespace '" << ns << "':\n";
                            for (const auto & tok : tokens) {
                                std::cerr << tok.dump();
                            }
                        }
                        parser->parseScript(tokens, file_content, file);
                        if (debugParser_) {
                            std::cerr << "[Debug][Parser] Operations for namespace '" << ns << "':\n";
                            for (const auto & op : Operations::Container::instance()->getAll(ns)) {
                                std::cerr << op->toString() << "\n";
                            }
                        }
                        Interpreter::Interpreter interpreter(debugInterpreter_);
                        interpreter.run();
                        // Clear operations after execution to avoid re-running
                        Operations::Container::instance()->clear(ns);
                        if (debugSymbolTable_) {
                            std::cout << Symbols::SymbolContainer::dump() << "\n";
                        }
                    }
                }
            }  // while (!files.empty())

            return 0;
        } catch (const std::exception & e) {
            std::cerr << e.what() << '\n';
            return 1;
        }
        return 1;
    }
};  // class VoidScript

#endif  // VOIDSCRIPT_HPP
