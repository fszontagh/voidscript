#ifndef MODULES_IMODULECONTEXT_HPP
#define MODULES_IMODULECONTEXT_HPP

#include <functional>
#include <string>

#include "Symbols/Value.hpp"

namespace Modules {

using FunctionArguments = const std::vector<Symbols::Value>;
using CallbackFunction  = std::function<Symbols::Value(FunctionArguments &)>;

/**
 * @brief Documentation structure for function parameters.
 */
struct FunctParameterInfo {
    std::string              name;                 // the name of the parameter
    Symbols::Variables::Type type;                 // the type of the parameter
    std::string              description;          // the description of the parameter
    bool                     optional    = false;  // if the parameter is optional
    bool                     interpolate = false;  // if the parameter is interpolated
};

/**
 * @brief Documentation structure for module functions.
 */
struct FunctionDoc {
    // the name of the function / method
    std::string                     name;
    Symbols::Variables::Type        returnType;
    // list of the parameters, empty if no parameters required
    std::vector<FunctParameterInfo> parameterList;
    // short description of the function / method
    std::string                     description;
};

/**
 * @brief Context interface provided to modules during registration.
 */
class IModuleContext {
  public:
    virtual ~IModuleContext() = default;

    /**
     * @brief Register a function with the given name and callback.
     */
    virtual void registerFunction(
        const std::string & name, CallbackFunction cb,
        const Symbols::Variables::Type & returnType = Symbols::Variables::Type::NULL_TYPE) = 0;

    /**
     * @brief Register documentation for a function.
     */
    virtual void registerDoc(const std::string & modName, const FunctionDoc & doc) = 0;

    /**
     * @brief Get the module's current name or identity, optional.
     */
    virtual std::string getCurrentModuleName() const = 0;
};

}  // namespace Modules

#endif  // MODULES_IMODULECONTEXT_HPP
