// CurlModule: declares a module that provides 'curl' function via libcurl
#ifndef FORMATMODULE_HPP
#define FORMATMODULE_HPP

#include <vector>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

class FormatModule : public BaseModule {
  public:
    /**
     * @brief Register this module's symbols (HTTP GET and POST functions).
     */
    void registerModule() override;
};

}  // namespace Modules

#endif  // FORMATMODULE_HPP
