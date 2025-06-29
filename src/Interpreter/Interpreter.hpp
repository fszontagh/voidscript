#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "BaseException.hpp"
#include "Interpreter/Operation.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

// Forward declaration for node types used in Visit methods
namespace Nodes::Statement {
    class EnumDeclarationNode;
    class SwitchStatementNode;
    class BreakNode;
    // Add other node forward declarations here as needed for Visit methods
}

namespace Interpreter {

class Exception : public BaseException {
  public:
    Exception(const std::string & msg, const std::string & filename, int line, size_t column) {
        rawMessage_ = msg;
        if (filename == "-") {
            context_ = "At line: " + std::to_string(line) + ", column: " + std::to_string(column);
        } else {
            context_ = std::string(" in file \"") + filename + "\" at line: " + std::to_string(line) +
                       ", column: " + std::to_string(column);
        }
        formattedMessage_ = formatMessage();
    }

    std::string formatMessage() const override {
        return std::string("[Runtime ERROR] >>") + context_ + " << : " + rawMessage_;
    }
};

class Interpreter {
  private:
    bool debug_ = false;
    static inline unsigned long long next_call_id_ = 0;
    Symbols::ValuePtr thisObject_;  // Current "this" object for method calls
    std::string currentClassName_;  // Current class context for method execution

  public:
    /**
     * @brief Construct interpreter with optional debug output
     * @param debug enable interpreter debug output
     */
    Interpreter(bool debug = false) : debug_(debug) {}

    /**
     * @brief Sets the current "this" object for method calls
     * @param obj The object to set as "this"
     */
    void setThisObject(const Symbols::ValuePtr& obj);

    /**
     * @brief Clears the current "this" object
     */
    void clearThisObject();

    /**
     * @brief Gets the current "this" object
     * @return The current "this" object or empty ValuePtr if none is set
     */
    const Symbols::ValuePtr& getThisObject() const;

    /**
     * @brief Sets the current class context for method execution
     * @param className The name of the class whose method is being executed
     */
    void setCurrentClass(const std::string& className);

    /**
     * @brief Clears the current class context
     */
    void clearCurrentClass();

    /**
     * @brief Gets the current class context
     * @return The current class name or empty string if not in a class method
     */
    const std::string& getCurrentClass() const;

    /**
     * @brief Check if access to a private member should be allowed
     * @param targetClassName The class that owns the member being accessed
     * @param memberName The name of the member being accessed
     * @param isProperty True if accessing a property, false if accessing a method
     * @return True if access should be allowed, false otherwise
     */
    bool canAccessPrivateMember(const std::string& targetClassName,
                               const std::string& memberName,
                               bool isProperty) const;

    /**
     * @brief Execute a method on an object
     * @param objectValue The object instance or name
     * @param methodName The name of the method to call
     * @param args Vector of arguments to pass to the method
     * @return The result of the method call
     * @throws Interpreter::Exception if method not found or execution fails
     */
    Symbols::ValuePtr executeMethod(const Symbols::ValuePtr& objectValue, 
                                  const std::string& methodName,
                                  const std::vector<Symbols::ValuePtr>& args);

    /**
     * @brief Get a unique identifier for function/method calls
     * @return A unique ID for the current call
     */
    static unsigned long long get_unique_call_id();

    /**
     * @brief Execute all operations in the current namespace
     * Executes operations at file-level or function-level scope
     */
    void run();

    /**
     * @brief Execute a single operation
     * @param op The operation to execute
     * @throws Interpreter::Exception if operation execution fails
     */
    void runOperation(const Operations::Operation& op);

    // Visitor methods for AST nodes
    // Add other Visit methods here as they are implemented

};  // class Interpreter

}  // namespace Interpreter
#endif  // INTERPRETER_HPP
