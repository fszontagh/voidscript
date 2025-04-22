// MariaDBModule.hpp
#ifndef MARIADBMODULE_MARIADBMODULE_HPP
#define MARIADBMODULE_MARIADBMODULE_HPP

#include <vector>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief Module providing basic MariaDB class with connect, query, close methods.
 */
class MariaDBModule : public BaseModule {
  public:
    void registerModule() override;

  private:
    // Methods exposed for MariaDB class
    Symbols::Value connect(FuncionArguments & args);
    Symbols::Value query(FuncionArguments & args);
    Symbols::Value close(FuncionArguments & args);
    Symbols::Value insert(FuncionArguments & args);
};

}  // namespace Modules

#endif  // MARIADBMODULE_MARIADBMODULE_HPP
