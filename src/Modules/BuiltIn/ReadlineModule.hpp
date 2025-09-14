#ifndef MODULES_READLINEMODULE_HPP
#define MODULES_READLINEMODULE_HPP

// Only compile this module when building for CLI
#ifdef CLI

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <limits>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <termios.h>
    #include <unistd.h>
    #include <sys/select.h>
#endif

#include "../BaseModule.hpp"
#include "../../Symbols/SymbolContainer.hpp"
#include "../../Symbols/Value.hpp"
#include "../../Symbols/RegistrationMacros.hpp"

namespace Modules {

/**
 * @brief Readline module providing terminal input functions for VoidScript CLI
 *
 * This module is only available when VoidScript is built for CLI usage.
 *
 * Provides readline functions:
 * - readline(prompt) -> read a line of input with an optional prompt
 * - readchar() -> read a single character from input
 * - getline() -> read a line without a prompt
 */
class ReadlineModule : public BaseModule {
  private:
    // Caching mechanism to handle VoidScript engine calling functions twice
    // This is a workaround for an engine-level issue where input functions
    // are invoked multiple times for a single script call
    static thread_local std::string lastReadlineResult;
    static thread_local std::string lastReadlinePrompt;
    static thread_local bool hasValidReadlineCache;
    
    static thread_local std::string lastGetlineResult;
    static thread_local bool hasValidGetlineCache;
    
    static thread_local std::string lastReadcharResult;
    static thread_local bool hasValidReadcharCache;
  public:
    ReadlineModule() {
        setModuleName("Readline");
        setDescription("Provides terminal input functions for reading user input in CLI mode");
        setBuiltIn(true);
    }

    void registerFunctions() override {
        // readline function with optional prompt
        std::vector<Symbols::FunctionParameterInfo> readline_params = {
            { "prompt", Symbols::Variables::Type::STRING, "The prompt to display (optional)", true, false }
        };
        REGISTER_FUNCTION("readline", Symbols::Variables::Type::STRING, readline_params,
                          "Read a line of input from the user with an optional prompt",
                          &ReadlineModule::Readline);

        // readchar function - reads a single character
        std::vector<Symbols::FunctionParameterInfo> no_params = {};
        REGISTER_FUNCTION("readchar", Symbols::Variables::Type::STRING, no_params,
                          "Read a single character from input without requiring Enter",
                          &ReadlineModule::Readchar);

        // getline function - reads a line without prompt
        REGISTER_FUNCTION("getline", Symbols::Variables::Type::STRING, no_params,
                          "Read a line of input without any prompt",
                          &ReadlineModule::Getline);
    }

    static Symbols::ValuePtr Readline(const FunctionArguments& args) {
        std::string prompt = "";
        if (!args.empty()) {
            if (args[0].getType() != Symbols::Variables::Type::STRING) {
                throw Exception("readline: prompt must be a string");
            }
            prompt = args[0].get<std::string>();
        }
        
        // Check if we have a valid cache entry for this exact prompt
        if (hasValidReadlineCache && lastReadlinePrompt == prompt) {
            // Clear the cache to prevent reuse and return cached result
            hasValidReadlineCache = false;
            return lastReadlineResult;
        }
        
        try {
            std::string result = readLineWithPrompt(prompt);
            
            // Cache the result
            lastReadlineResult = result;
            lastReadlinePrompt = prompt;
            hasValidReadlineCache = true;
            
            return result;
        } catch (const std::exception& e) {
            hasValidReadlineCache = false; // Clear cache on error
            throw Exception("readline: " + std::string(e.what()));
        }
    }

    static Symbols::ValuePtr Readchar(const FunctionArguments& args) {
        if (!args.empty()) {
            throw Exception("readchar expects no arguments");
        }
        
        // Check if we have a valid cache entry
        if (hasValidReadcharCache) {
            // Clear the cache to prevent reuse and return cached result
            hasValidReadcharCache = false;
            return lastReadcharResult;
        }
        
        try {
            std::string result = readSingleChar();
            
            // Cache the result
            lastReadcharResult = result;
            hasValidReadcharCache = true;
            
            return result;
        } catch (const std::exception& e) {
            hasValidReadcharCache = false; // Clear cache on error
            throw Exception("readchar: " + std::string(e.what()));
        }
    }

    static Symbols::ValuePtr Getline(const FunctionArguments& args) {
        if (!args.empty()) {
            throw Exception("getline expects no arguments");
        }
        
        // Check if we have a valid cache entry
        if (hasValidGetlineCache) {
            // Clear the cache to prevent reuse and return cached result
            hasValidGetlineCache = false;
            return lastGetlineResult;
        }
        
        try {
            std::string result = readLineWithoutPrompt();
            
            // Cache the result
            lastGetlineResult = result;
            hasValidGetlineCache = true;
            
            return result;
        } catch (const std::exception& e) {
            hasValidGetlineCache = false; // Clear cache on error
            throw Exception("getline: " + std::string(e.what()));
        }
    }

  private:

    /**
     * @brief Read a line of input with an optional prompt
     * @param prompt The prompt to display before reading input
     * @return The line read from input as a string
     */
    static std::string readLineWithPrompt(const std::string& prompt) {
        if (!prompt.empty()) {
            std::cout << prompt;
            std::cout.flush();
        }
        
        std::string line;
        if (!std::getline(std::cin, line)) {
            // Handle EOF or input error
            if (std::cin.eof()) {
                throw std::runtime_error("End of file reached");
            } else {
                // Clear error state and try to continue
                std::cin.clear();
                throw std::runtime_error("Input error occurred");
            }
        }
        
        return line;
    }

    /**
     * @brief Read a line of input without any prompt
     * @return The line read from input as a string
     */
    static std::string readLineWithoutPrompt() {
        std::string line;
        if (!std::getline(std::cin, line)) {
            // Handle EOF or input error
            if (std::cin.eof()) {
                throw std::runtime_error("End of file reached");
            } else {
                // Clear error state and try to continue
                std::cin.clear();
                throw std::runtime_error("Input error occurred");
            }
        }
        
        return line;
    }

    /**
     * @brief Read a single character from input without requiring Enter
     * @return Single character as a string
     */
    static std::string readSingleChar() {
#ifdef _WIN32
        // Windows implementation using conio.h
        std::cout.flush();
        
        int ch = _getch();
        if (ch == EOF) {
            throw std::runtime_error("End of file reached");
        }
        return std::string(1, static_cast<char>(ch));
#else
        // Unix/Linux implementation using termios
        struct termios old_tio, new_tio;
        
        std::cout.flush();
        
        // Get current terminal settings
        if (tcgetattr(STDIN_FILENO, &old_tio) != 0) {
            throw std::runtime_error("Failed to get terminal attributes");
        }
        
        // Set up new terminal settings for raw input
        new_tio = old_tio;
        new_tio.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
        new_tio.c_cc[VMIN] = 1;  // Minimum number of characters for read
        new_tio.c_cc[VTIME] = 0; // Timeout in deciseconds for read
        
        // Apply new settings
        if (tcsetattr(STDIN_FILENO, TCSANOW, &new_tio) != 0) {
            throw std::runtime_error("Failed to set terminal attributes");
        }
        
        char ch;
        ssize_t result = read(STDIN_FILENO, &ch, 1);
        
        // Restore original terminal settings
        tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
        
        if (result <= 0) {
            if (result == 0) {
                throw std::runtime_error("End of file reached");
            } else {
                throw std::runtime_error("Failed to read character");
            }
        }
        
        return std::string(1, ch);
#endif
    }
};

// Thread-local cache variables definitions
thread_local std::string ReadlineModule::lastReadlineResult = "";
thread_local std::string ReadlineModule::lastReadlinePrompt = "";
thread_local bool ReadlineModule::hasValidReadlineCache = false;

thread_local std::string ReadlineModule::lastGetlineResult = "";
thread_local bool ReadlineModule::hasValidGetlineCache = false;

thread_local std::string ReadlineModule::lastReadcharResult = "";
thread_local bool ReadlineModule::hasValidReadcharCache = false;

} // namespace Modules

#endif // CLI

#endif // MODULES_READLINEMODULE_HPP