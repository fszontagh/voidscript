// BaseModule.hpp
#ifndef MODULES_BASEMODULE_HPP
#define MODULES_BASEMODULE_HPP

// Base exception type for module errors
#include <vector>

#include "../BaseException.hpp"
#include "Symbols/Value.hpp"

namespace Modules {
using FuncionArguments = const std::vector<Symbols::Value>;

template <typename T> static std::map<int, T> typeHolder;
static int                                    typeCounter = 0;

/**
 * @brief Base class for modules that can register functions and variables into the symbol table.
 */
class BaseModule {
  protected:
    std::string moduleName;
  public:
    BaseModule()          = default;
    virtual ~BaseModule() = default;

    /**
     * @brief Register this module's symbols (functions, variables) into the global symbol container.
     * Modules should use Symbols::SymbolContainer::instance() and SymbolFactory to add symbols.
     */
    virtual void registerModule() = 0;

    Symbols::Value::ObjectMap getObjectMap(const FuncionArguments & args, const std::string & funcName) {
        if (args.size() > 0) {
            if (args[0].getType() != Symbols::Variables::Type::CLASS &&
                args[0].getType() != Symbols::Variables::Type::OBJECT) {
                throw std::runtime_error(this->moduleName + "::" + funcName + " must be called on " + this->moduleName +
                                         " instance");
            }
            return std::get<Symbols::Value::ObjectMap>(args[0].get());
        }
        throw std::invalid_argument(this->moduleName + "::" + funcName + ": invalid arguments size");
    }

    template <typename T> int storeType(const T & data) {
        const auto currentId       = typeCounter;
        typeHolder<T>[typeCounter] = data;
        typeCounter++;
        return currentId;
    }

    template <typename T> T getType(int i) {
        auto it = typeHolder<T>.find(i);
        if (it != typeHolder<T>.end()) {
            return it->second;
        }
        throw std::runtime_error("Data not found at index: " + std::to_string(i));
    }

    Symbols::Value::ObjectMap storeObject(const FuncionArguments & args, const Symbols::Value & value,
                                          const std::string & objName = "__item__") {
        auto objectMap     = this->getObjectMap(args, "");
        objectMap[objName] = value;
        return objectMap;
    }

    Symbols::Value getObjectValue(const FuncionArguments & args, const std::string & objName = "__item__") {
        auto objectMap = this->getObjectMap(args, objName);
        auto it        = objectMap.find(objName);
        if (it == objectMap.end()) {
            throw std::runtime_error("Object not found in objectMap: " + objName);
        }
        return it->second;
    }
};

/**
 * @brief Exception type for errors thrown within module functions.
 * Inherit from BaseException to allow rich error messages.
 */
class Exception : public ::BaseException {
  public:
    /**
     * Construct a module exception with a message.
     * @param msg Error message
     */
    explicit Exception(const std::string & msg) : BaseException(msg) {}
};

}  // namespace Modules
#endif  // MODULES_BASEMODULE_HPP
