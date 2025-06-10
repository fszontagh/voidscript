#include "Compiler/VoidScriptCompiler.hpp"
#include "Interpreter/OperationsFactory.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Symbols/SymbolContainer.hpp"

#include <iostream>

/**
 * @brief Example demonstrating the VoidScript compiler infrastructure
 */
int main() {
    std::cout << "VoidScript Compiler Infrastructure Demo\n";
    std::cout << "=====================================\n\n";
    
    try {
        // Create compilation options
        auto options = Compiler::VoidScriptCompiler::createDebugOptions();
        options.outputPath = "example_output";
        
        // Create compiler instance
        Compiler::VoidScriptCompiler compiler(options);
        
        std::cout << "Compiler Version: " << Compiler::VoidScriptCompiler::getVersion() << "\n\n";
        
        // Initialize symbol container (simulating parsed program)
        auto* symbolContainer = Symbols::SymbolContainer::instance();
        symbolContainer->pushScope("main");
        
        // Add some example operations using OperationsFactory
        std::cout << "Adding example operations...\n";
        
        // Example 1: Variable declaration
        auto stringValue = std::make_shared<Symbols::Value>();
        stringValue->set(std::string("Hello, World!"));
        Interpreter::OperationsFactory::defineSimpleVariable(
            "message", 
            Symbols::ValuePtr(std::move(*stringValue)), 
            "main", 
            "example.vs", 
            1, 
            0
        );
        
        // Example 2: Function call
        std::vector<Parser::ParsedExpressionPtr> args; // Empty args for now
        Interpreter::OperationsFactory::callFunction(
            "print", 
            std::move(args), 
            "main", 
            "example.vs", 
            2, 
            0
        );
        
        // Example 3: Function declaration
        std::vector<Symbols::FunctionParameterInfo> params;
        Interpreter::OperationsFactory::defineFunction(
            "hello", 
            params, 
            Symbols::Variables::Type::STRING, 
            "main", 
            "example.vs", 
            3, 
            0
        );
        
        std::cout << "Operations added to container.\n\n";
        
        // Display operations that will be compiled
        std::cout << "Operations to compile:\n";
        std::cout << Operations::Container::dump() << "\n";
        
        // Compile the operations
        std::cout << "Starting compilation...\n";
        bool success = compiler.compileOperations();
        
        if (success) {
            std::cout << "✓ Compilation successful!\n\n";
            
            // Show generated assembly code
            auto assemblyCode = compiler.getAssemblyCode();
            std::cout << "Generated Assembly Code:\n";
            std::cout << "========================\n";
            for (const auto& line : assemblyCode) {
                std::cout << line << "\n";
            }
            std::cout << "\n";
            
            std::cout << "Output written to: " << compiler.getOutputPath() << "\n";
        } else {
            std::cout << "✗ Compilation failed!\n\n";
            
            // Show error messages
            auto messages = compiler.getMessages();
            std::cout << "Error Messages:\n";
            std::cout << "===============\n";
            for (const auto& message : messages) {
                std::cout << message << "\n";
            }
        }
        
        // Clean up
        symbolContainer->popScope();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nDemo completed.\n";
    return 0;
}