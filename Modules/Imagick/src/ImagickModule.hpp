// ImagickModule: declares a module that provides 'imagick' function via imagick
#ifndef IMAGICK_NODULE_HPP
#define IMAGICK_NODULE_HPP
#include <Magick++.h>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

class ImagickModule : public BaseModule {
  public:
    /**
     * @brief Register this module's symbols
     */
    void registerModule() override;

  private:
    std::unordered_map<int, Magick::Image> images_;
    unsigned int                           next_image_id = 0;
    Symbols::Value                         read(FuncionArguments & args);
    Symbols::Value                         crop(FuncionArguments & args);
    Symbols::Value                         resize(FuncionArguments & args);
    Symbols::Value                         write(FuncionArguments & args);
    Symbols::Value                         mode(FuncionArguments & args);
    Symbols::Value                         blur(FuncionArguments & args);
    Symbols::Value                         rotate(FuncionArguments & args);
    Symbols::Value                         flip(FuncionArguments & args);
    Symbols::Value                         getWidth(FuncionArguments & args);
    Symbols::Value                         getHeight(FuncionArguments & args);
    Symbols::Value                         composite(FuncionArguments & args);

};  // Class ImagickModule
}  // namespace Modules
#endif  // IMAGICK_NODULE_HPP
