#ifndef MODULES_BASEMODULE_HPP
#define MODULES_BASEMODULE_HPP

#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../BaseException.hpp"
#include "Symbols/Value.hpp"

// Forward declaration to avoid circular dependency
namespace Symbols {
    class SymbolContainer;
}

using FunctionArguments = const std::vector<Symbols::ValuePtr>;
using CallbackFunction  = std::function<Symbols::ValuePtr(FunctionArguments &)>;

namespace Modules {

// Global template instance for type storage
template <typename T> static std::map<int, T> typeHolder;
static int                                    typeCounter = 0;

/**
 * @brief Base class for modules that register symbols into the symbol table.
 */
class BaseModule {
  protected:
    std::string moduleName;
    std::string moduleDescription;
    bool        isBuiltIn_ = false;  // Flag to indicate if this is a built-in module (default: false for external)

  public:
    BaseModule() = default;
    virtual ~BaseModule() = default;

    /**
     * @brief Register module functions with the SymbolContainer
     * This method should be implemented by each module to register its functions
     */
    virtual void registerFunctions() = 0;

    void setModuleName(const std::string & name) { this->moduleName = name; }

    std::string name() const { return this->moduleName; }

    /**
     * @brief Set the description for this module
     * @param description The description of what this module provides
     */
    void setDescription(const std::string & description) { this->moduleDescription = description; }

    /**
     * @brief Get the description of this module
     * @return The module description
     */
    std::string description() const { return this->moduleDescription; }

    /**
     * @brief Set whether this module is built-in
     * @param builtIn True if built-in, false if external
     */
    void setBuiltIn(bool builtIn) { this->isBuiltIn_ = builtIn; }

    /**
     * @brief Check if this module is built-in
     * @return True if built-in, false otherwise (external)
     */
    bool isBuiltIn() const { return this->isBuiltIn_; }

    Symbols::ObjectMap getObjectMap(const FunctionArguments & args, const std::string & funcName) {
        constexpr const char* SCOPE_SEP = "::";  // Local definition to avoid circular dependency
        if (!args.empty()) {
            if (args[0].getType() != Symbols::Variables::Type::CLASS &&
                args[0].getType() != Symbols::Variables::Type::OBJECT) {
                throw std::runtime_error(this->moduleName + SCOPE_SEP + funcName +
                                         " must be called on " + this->moduleName + " instance");
            }
            return args[0];
        }
        throw std::invalid_argument(this->moduleName + SCOPE_SEP + funcName + ": invalid arguments size");
    }

    template <typename T> int storeType(const T & data) {
        const auto currentId       = typeCounter;
        typeHolder<T>[typeCounter] = data;
        ++typeCounter;
        return currentId;
    }

    template <typename T> T getType(int i) {
        auto it = typeHolder<T>.find(i);
        if (it != typeHolder<T>.end()) {
            return it->second;
        }
        throw std::runtime_error("Data not found at index: " + std::to_string(i));
    }

    Symbols::ObjectMap storeObject(const FunctionArguments & args, Symbols::ValuePtr value,
                                   const std::string & objName = "__item__") {
        auto objectMap     = this->getObjectMap(args, "");
        objectMap[objName] = std::move(value);
        return objectMap;
    }

    Symbols::ValuePtr getObjectValue(const FunctionArguments & args, const std::string & objName = "__item__") {
        auto objectMap = this->getObjectMap(args, objName);
        auto it        = objectMap.find(objName);
        if (it == objectMap.end()) {
            throw std::runtime_error("Object not found in objectMap: " + objName);
        }
        return it->second;
    }
};

/**
 * @brief Module exception type with detailed error messages.
 */
class Exception : public ::BaseException {
  public:
    explicit Exception(const std::string & msg) : BaseException(msg) {}
};

}  // namespace Modules

#endif  // MODULES_BASEMODULE_HPP
