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
    void registerFunctions() override;
    MariaDBModule() { this->setModuleName("MariaDB"); }
    ~MariaDBModule() override = default;
  private:
    
    // Methods exposed for MariaDB class
    Symbols::ValuePtr connect(FunctionArguments & args);
    Symbols::ValuePtr query(FunctionArguments & args);
    Symbols::ValuePtr close(FunctionArguments & args);
    Symbols::ValuePtr insert(FunctionArguments & args);
};

}  // namespace Modules

#endif  // MARIADBMODULE_MARIADBMODULE_HPP
