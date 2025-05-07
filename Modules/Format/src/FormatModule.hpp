// CurlModule: declares a module that provides 'curl' function via libcurl
#ifndef FORMATMODULE_HPP
#define FORMATMODULE_HPP

#include "Modules/BaseModule.hpp"

namespace Modules {

class FormatModule : public BaseModule {
  public:
    FormatModule() { setModuleName("Format"); }

    /**
     * @brief Register this module's symbols
     */
    void registerModule(IModuleContext & context);
};

}  // namespace Modules

#endif  // FORMATMODULE_HPP
