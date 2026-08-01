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
    Symbols::ValuePtr                      getPixel(FunctionArguments & args);
    Symbols::ValuePtr                      setPixel(FunctionArguments & args);

    // Shared handle lookup. Every method repeated the same three-step dance of
    // objectId -> handle -> image with its own error strings.
    Magick::Image &                        imageFor(FunctionArguments & args, const char * method,
                                                            size_t argIndex = 0);
    Symbols::ValuePtr                      composite(FunctionArguments & args);
    Symbols::ValuePtr                      newImage(FunctionArguments & args);
    Symbols::ValuePtr                      gradient(FunctionArguments & args);
    Symbols::ValuePtr                      radialGradient(FunctionArguments & args);
    // Shared body for gradient()/radialGradient(): render a native gradient pseudo-image
    // (`pseudo` is "gradient" or "radial-gradient") and stamp a fresh handle.
    Symbols::ValuePtr                      makeGradient(FunctionArguments & args, const char * method,
                                                        const char * pseudo);
    Symbols::ValuePtr                      extent(FunctionArguments & args);
    Symbols::ValuePtr                      addNoise(FunctionArguments & args);
    Symbols::ValuePtr                      evaluate(FunctionArguments & args);
    Symbols::ValuePtr                      compositeMultiply(FunctionArguments & args);
    Symbols::ValuePtr                      compositeOp(FunctionArguments & args);
    Symbols::ValuePtr                      distort(FunctionArguments & args);
    Symbols::ValuePtr                      extractChannel(FunctionArguments & args);
    Symbols::ValuePtr                      combineChannels(FunctionArguments & args);
    Symbols::ValuePtr                      stripImage(FunctionArguments & args);
};

}  // namespace Modules

#endif  // IMAGICK_MODULE_HPP
