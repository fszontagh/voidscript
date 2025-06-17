// FormatModule: declares a module that provides string formatting via fmt library
#ifndef FORMATMODULE_HPP
#define FORMATMODULE_HPP

#include "Modules/BaseModule.hpp"

namespace Modules {

class FormatModule : public BaseModule {
  public:
    FormatModule() {
        setModuleName("format");
        setDescription("Provides advanced string formatting capabilities using the fmt library, supporting placeholder-based string interpolation and formatted output similar to Python's format() function");
    }

    /**
     * @brief Register this module's functions
     */
    void registerFunctions() override;
};

}  // namespace Modules

#endif  // FORMATMODULE_HPP
