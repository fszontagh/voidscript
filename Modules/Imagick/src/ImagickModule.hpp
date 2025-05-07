// ImagickModule.hpp
#ifndef IMAGICK_MODULE_HPP
#define IMAGICK_MODULE_HPP

#include <Magick++.h>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

class ImagickModule : public BaseModule {
  public:
    ImagickModule() { setModuleName("Imagick"); }

    /**
     * @brief Register this module's symbols
     */
    void registerModule(IModuleContext & context) override;

  private:
    std::unordered_map<int, Magick::Image> images_;
    unsigned int                           next_image_id = 0;
    Symbols::Value                         read(FunctionArguments & args);
    Symbols::Value                         crop(FunctionArguments & args);
    Symbols::Value                         resize(FunctionArguments & args);
    Symbols::Value                         write(FunctionArguments & args);
    Symbols::Value                         mode(FunctionArguments & args);
    Symbols::Value                         blur(FunctionArguments & args);
    Symbols::Value                         rotate(FunctionArguments & args);
    Symbols::Value                         flip(FunctionArguments & args);
    Symbols::Value                         getWidth(FunctionArguments & args);
    Symbols::Value                         getHeight(FunctionArguments & args);
    Symbols::Value                         composite(FunctionArguments & args);
};

}  // namespace Modules

#endif  // IMAGICK_MODULE_HPP
