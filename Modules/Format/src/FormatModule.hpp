// CurlModule: declares a module that provides 'curl' function via libcurl
#ifndef FORMATMODULE_HPP
#define FORMATMODULE_HPP

#include "Modules/BaseModule.hpp"

namespace Modules {

class FormatModule : public BaseModule {
  public:
    FormatModule() { setModuleName("format"); }

    /**
     * @brief Register this module's symbols
     */
    void registerModule();
};

}  // namespace Modules

#endif  // FORMATMODULE_HPP
