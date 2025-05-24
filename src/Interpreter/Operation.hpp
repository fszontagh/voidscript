#ifndef INTERPRETER_OPERATION_HPP
#define INTERPRETER_OPERATION_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "StatementNode.hpp"

namespace Operations {
enum class Type : std::uint8_t {
    Assignment,       // Variable assignment, e.g., $x = 5
    Expression,       // Evaluation of an expression (can be evaluated without side effects)
    FunctionCall,     // Call to a function, e.g., print(x)
    MethodCall,       // Call to an object method, e.g., $obj->method()
    FuncDeclaration,  // declaration of new function
    MethodDeclaration,// declaration of a class method, e.g., function xyz() in class ABC
    Return,           // return statement
    Conditional,      // if/else structure
    Loop,             // while/for loop
    While,            // While loop
    Break,            // break out of a loop
    Continue,         // continue with the next iteration of a loop
    Block,            // block of statements, e.g. { ... }
    Declaration,      // declaration of new variable (if different from assignment) int $x = 1
    Import,           // import of another script or module
    Error,            // error or non-interpretable operation (error handling)
};

struct Operation {
    Operation() = default;
    Operations::Type type;

    // Általános mezők
    std::string                                 targetName;
    std::unique_ptr<Interpreter::StatementNode> statement;

    Operation(Operations::Type t, std::string target_variable, std::unique_ptr<Interpreter::StatementNode> stmt) :
        type(t),
        targetName(std::move(target_variable)),
        statement(std::move(stmt)) {}

    std::string typeToString() const {
        std::unordered_map<Operations::Type, std::string> types = {
            { Operations::Type::Assignment,      "Assignment"      },
            { Operations::Type::Expression,      "Expression"      },
            { Operations::Type::FunctionCall,    "FunctionCall"    },
            { Operations::Type::MethodCall,      "MethodCall"      },
            { Operations::Type::FuncDeclaration, "FuncDeclaration" },
            { Operations::Type::MethodDeclaration, "MethodDeclaration" },
            { Operations::Type::Return,          "Return"          },
            { Operations::Type::Conditional,     "Conditional"     },
            { Operations::Type::Loop,            "Loop"            },
            { Operations::Type::Break,           "Break"           },
            { Operations::Type::Continue,        "Continue"        },
            { Operations::Type::Block,           "Block"           },
            { Operations::Type::Declaration,     "Declaration"     },
            { Operations::Type::Import,          "Import"          },
            { Operations::Type::Error,           "Error"           }
        };
        auto it = types.find(type);
        if (it != types.end()) {
            return it->second;
        }
        return "Unknown type";
    }

    std::string toString() const {
        return "Target: " + targetName + " Type: " + this->typeToString() +
               " Statement: " + ((statement == nullptr) ? "no statement" : statement->toString());
    }
};
};  // namespace Operations
#endif
