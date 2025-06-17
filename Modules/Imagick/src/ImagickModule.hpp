// ImagickModule.hpp
#ifndef IMAGICK_MODULE_HPP
#define IMAGICK_MODULE_HPP

#include <Magick++.h>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

class ImagickModule : public BaseModule {
  public:
    ImagickModule() {
        setModuleName("Imagick");
        setDescription("Provides comprehensive image processing capabilities using ImageMagick library, including reading, writing, resizing, cropping, rotating, blurring, and various image manipulation operations");
    }

    /**
     * @brief Register this module's functions
     */
    void registerFunctions() override;

  private:
    std::unordered_map<int, Magick::Image> images_;
    static std::unordered_map<std::string, int> object_to_handle_map_; // Map object identity to image handle
    unsigned int                           next_image_id = 0;
    Symbols::ValuePtr                      construct(FunctionArguments & args);
    Symbols::ValuePtr                      read(FunctionArguments & args);
    Symbols::ValuePtr                      crop(FunctionArguments & args);
    Symbols::ValuePtr                      resize(FunctionArguments & args);
    Symbols::ValuePtr                      write(FunctionArguments & args);
    Symbols::ValuePtr                      mode(FunctionArguments & args);
    Symbols::ValuePtr                      blur(FunctionArguments & args);
    Symbols::ValuePtr                      rotate(FunctionArguments & args);
    Symbols::ValuePtr                      flip(FunctionArguments & args);
    Symbols::ValuePtr                      getWidth(FunctionArguments & args);
    Symbols::ValuePtr                      getHeight(FunctionArguments & args);
    Symbols::ValuePtr                      composite(FunctionArguments & args);
};

}  // namespace Modules

#endif  // IMAGICK_MODULE_HPP
