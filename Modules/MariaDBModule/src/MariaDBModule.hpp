// MariaDBModule.hpp
#ifndef MARIADBMODULE_MARIADBMODULE_HPP
#define MARIADBMODULE_MARIADBMODULE_HPP

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"
#include <vector>

namespace Modules {

/**
 * @brief Module providing basic MariaDB class with connect, query, close methods.
 */
class MariaDBModule : public BaseModule {
  public:
    void registerModule() override;

  private:
    // Methods exposed for MariaDB class
    Symbols::Value connect(const std::vector<Symbols::Value> & args);
    Symbols::Value query(const std::vector<Symbols::Value> & args);
    Symbols::Value close(const std::vector<Symbols::Value> & args);
};

} // namespace Modules

#endif // MARIADBMODULE_MARIADBMODULE_HPP