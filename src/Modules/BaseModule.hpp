// BaseModule.hpp
#ifndef MODULES_BASEMODULE_HPP
#define MODULES_BASEMODULE_HPP

// Base exception type for module errors
#include <vector>

#include "../BaseException.hpp"
#include "Symbols/Value.hpp"

namespace Modules {
using FuncionArguments = const std::vector<Symbols::Value>;

/**
 * @brief Base class for modules that can register functions and variables into the symbol table.
 */
class BaseModule {
  public:
    BaseModule()          = default;
    virtual ~BaseModule() = default;

    /**
     * @brief Register this module's symbols (functions, variables) into the global symbol container.
     * Modules should use Symbols::SymbolContainer::instance() and SymbolFactory to add symbols.
     */
    virtual void registerModule() = 0;
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
