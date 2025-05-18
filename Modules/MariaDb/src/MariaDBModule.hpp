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
    void registerModule() override;

  private:
    // Methods exposed for MariaDB class
    Symbols::Value::ValuePtr connect(FunctionArguments & args);
    Symbols::Value::ValuePtr query(FunctionArguments & args);
    Symbols::Value::ValuePtr close(FunctionArguments & args);
    Symbols::Value::ValuePtr insert(FunctionArguments & args);
};

}  // namespace Modules

#endif  // MARIADBMODULE_MARIADBMODULE_HPP
