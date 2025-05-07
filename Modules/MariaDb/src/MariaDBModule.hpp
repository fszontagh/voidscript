// MariaDBModule.hpp
#ifndef MARIADBMODULE_MARIADBMODULE_HPP
#define MARIADBMODULE_MARIADBMODULE_HPP

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief Module providing basic MariaDB class with connect, query, close methods.
 */
class MariaDBModule : public BaseModule {
  public:
    void registerModule(IModuleContext & context) override;

  private:
    // Methods exposed for MariaDB class
    Symbols::Value connect(FunctionArguments & args);
    Symbols::Value query(FunctionArguments & args);
    Symbols::Value close(FunctionArguments & args);
    Symbols::Value insert(FunctionArguments & args);
};

}  // namespace Modules

#endif  // MARIADBMODULE_MARIADBMODULE_HPP
