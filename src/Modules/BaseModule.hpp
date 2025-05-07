#ifndef MODULES_BASEMODULE_HPP
#define MODULES_BASEMODULE_HPP

#include <map>
#include <stdexcept>
#include <string>

#include "../BaseException.hpp"
#include "IModuleContext.hpp"
#include "Symbols/Value.hpp"

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

  public:
    BaseModule()          = default;
    virtual ~BaseModule() = default;

    virtual void registerModule(IModuleContext & context) = 0;

    void setModuleName(const std::string & name) { this->moduleName = name; }

    std::string name() const { return this->moduleName; }

    Symbols::Value::ObjectMap getObjectMap(const FunctionArguments & args, const std::string & funcName) {
        if (!args.empty()) {
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

    Symbols::Value::ObjectMap storeObject(const FunctionArguments & args, const Symbols::Value & value,
                                          const std::string & objName = "__item__") {
        auto objectMap     = this->getObjectMap(args, "");
        objectMap[objName] = value;
        return objectMap;
    }

    Symbols::Value getObjectValue(const FunctionArguments & args, const std::string & objName = "__item__") {
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

/**
 * @brief Macro for registering module functions and documenting them using the provided context.
 */
#define REGISTER_MODULE_FUNCTION(context, modName, fnName, retType, paramListVec, docStr, lambda) \
    do {                                                                                          \
        (context).registerFunction(fnName, lambda, retType);                                        \
        (context).registerDoc(modName, FunctionDoc{ fnName, retType, paramListVec, docStr });       \
    } while (0)

}  // namespace Modules

#endif  // MODULES_BASEMODULE_HPP
