#ifndef PIXELART_MODULE_HPP
#define PIXELART_MODULE_HPP

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief VoidScript class "PixelArt" wrapping sdpixel2realpixelart.
 *
 * Turns AI "pixel-art-looking" images into real pixel art: detects the underlying pixel
 * grid, collapses each grid cell to one pixel and optionally quantizes the palette.
 *
 *   PixelArt $p = new PixelArt();
 *   string $out = $p->convert({ string input: "sprite.png", string output: "out.png",
 *                               int colors: 16, int scale_result: 4, boolean transparent: true });
 *
 * The transform is stateless (file in, file out), so an instance holds nothing; the class
 * form just matches the module system. See Modules/PixelArt/README.md for all option keys.
 */
class PixelArtModule : public BaseModule {
  public:
    PixelArtModule() {
        setModuleName("PixelArt");
        setDescription("Turn AI pixel-art-looking images into real pixel art (sdpixel2realpixelart)");
    }

    void registerFunctions() override;

  private:
    Symbols::ValuePtr construct(FunctionArguments & args);
    Symbols::ValuePtr convert(FunctionArguments & args);
};

}  // namespace Modules

#endif  // PIXELART_MODULE_HPP
