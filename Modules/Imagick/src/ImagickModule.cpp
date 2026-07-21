#include "ImagickModule.hpp"

#include <filesystem>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/Value.hpp"

void Modules::ImagickModule::registerFunctions() {
    std::vector<Symbols::FunctionParameterInfo> params = {
        { "filename", Symbols::Variables::Type::STRING, "The image file to manipulate" },
    };

    REGISTER_CLASS(this->name());

    // Register constructor
    REGISTER_METHOD(
        this->name(), "__construct", {}, [this](const FunctionArguments & args) { return this->construct(args); },
        Symbols::Variables::Type::CLASS, "Constructor for Imagick class");

    REGISTER_METHOD(
        this->name(), "read", params, [this](const FunctionArguments & args) { return this->read(args); },
        Symbols::Variables::Type::CLASS, "Read an image file");

    REGISTER_METHOD(
        this->name(), "write", params, [this](const FunctionArguments & args) { return this->write(args); },
        Symbols::Variables::Type::NULL_TYPE, "Save the image");

    params = {
        { "width",   Symbols::Variables::Type::INTEGER, "The width of the crop"                                    },
        { "height",  Symbols::Variables::Type::INTEGER, "The height of the crop"                                   },
        { "xOffset", Symbols::Variables::Type::INTEGER, "The X coordinate of the cropped region's top left corner" },
        { "yOffset", Symbols::Variables::Type::INTEGER, "The Y coordinate of the cropped region's top left corner" },
    };

    REGISTER_METHOD(
        this->name(), "crop", params, [this](const FunctionArguments & args) { return this->crop(args); },
        Symbols::Variables::Type::NULL_TYPE, "Extracts a region of the image");

    params = {
        { "width",  Symbols::Variables::Type::INTEGER, "The new width of the image" },
        { "height", Symbols::Variables::Type::INTEGER, "The new height of the image" }
    };

    REGISTER_METHOD(
        this->name(), "resize", params, [this](const FunctionArguments & args) { return this->resize(args); },
        Symbols::Variables::Type::NULL_TYPE, "Resize an image");

    params = {
        { "mode", Symbols::Variables::Type::STRING, "Colorspace: RGB, CMYK or GRAY" },
    };
    REGISTER_METHOD(
        this->name(), "mode", params, [this](const FunctionArguments & args) { return this->mode(args); },
        Symbols::Variables::Type::NULL_TYPE, "Set the image colorspace (RGB, CMYK, GRAY)");

    params = {
        { "radius", Symbols::Variables::Type::DOUBLE, "The blur radius" },
        { "sigma",  Symbols::Variables::Type::DOUBLE, "The standard deviation of the blur" },
    };

    REGISTER_METHOD(
        this->name(), "blur", params, [this](const FunctionArguments & args) { return this->blur(args); },
        Symbols::Variables::Type::NULL_TYPE, "Blur an image");

    params = {
        { "degrees", Symbols::Variables::Type::DOUBLE, "The angle in degrees to rotate the image" },
    };

    REGISTER_METHOD(
        this->name(), "rotate", params, [this](const FunctionArguments & args) { return this->rotate(args); },
        Symbols::Variables::Type::NULL_TYPE, "Rotate image");

    params = {
        { "direction", Symbols::Variables::Type::STRING, "The direction to flip the image (horizontal or vertical)" },
    };

    REGISTER_METHOD(
        this->name(), "flip", params, [this](const FunctionArguments & args) { return this->flip(args); },
        Symbols::Variables::Type::NULL_TYPE, "Flip image");
    REGISTER_METHOD(
        this->name(), "getWidth", {}, [this](const FunctionArguments & args) { return this->getWidth(args); },
        Symbols::Variables::Type::INTEGER, "Get the width of the image");
    REGISTER_METHOD(
        this->name(), "getHeight", {}, [this](const FunctionArguments & args) { return this->getHeight(args); },
        Symbols::Variables::Type::INTEGER, "Get the height of the image");
    params = { { "x", Symbols::Variables::Type::INTEGER, "X coordinate" },
               { "y", Symbols::Variables::Type::INTEGER, "Y coordinate" } };
    REGISTER_METHOD(
        this->name(), "getPixel", params, [this](const FunctionArguments & args) { return this->getPixel(args); },
        Symbols::Variables::Type::OBJECT, "Read a pixel as { red, green, blue, alpha }, each 0-255");
    params = { { "x", Symbols::Variables::Type::INTEGER, "X coordinate" },
               { "y", Symbols::Variables::Type::INTEGER, "Y coordinate" },
               { "red", Symbols::Variables::Type::INTEGER, "Red 0-255" },
               { "green", Symbols::Variables::Type::INTEGER, "Green 0-255" },
               { "blue", Symbols::Variables::Type::INTEGER, "Blue 0-255" },
               { "alpha", Symbols::Variables::Type::INTEGER, "Alpha 0-255", true } };
    REGISTER_METHOD(
        this->name(), "setPixel", params, [this](const FunctionArguments & args) { return this->setPixel(args); },
        Symbols::Variables::Type::NULL_TYPE, "Write a pixel from 0-255 channel values");
    params = { { "source", Symbols::Variables::Type::CLASS,   "Imagick image to overlay" },
               { "x",      Symbols::Variables::Type::INTEGER, "X offset of the overlay" },
               { "y",      Symbols::Variables::Type::INTEGER, "Y offset of the overlay" } };
    REGISTER_METHOD(
        this->name(), "composite", params, [this](const FunctionArguments & args) { return this->composite(args); },
        Symbols::Variables::Type::NULL_TYPE, "Overlay another image at (x, y) using the Over operator");
}

Symbols::ValuePtr Modules::ImagickModule::construct(FunctionArguments & args) {
    if (args.size() != 1) {
        throw std::runtime_error("Imagick::__construct expects no parameters, got: " + std::to_string(args.size() - 1));
    }

    if (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::__construct must be called on Imagick instance");
    }

    Symbols::ObjectMap objMap = args[0];
    
    // Initialize the object properly - no image loaded yet, so no __image_id__
    // The object is already created with $class_name, just return it as-is
    return Symbols::ValuePtr::makeClassInstance(objMap);
}

Symbols::ValuePtr Modules::ImagickModule::read(FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::runtime_error("Imagick::read expects (filename), got: " + std::to_string(args.size() - 1));
    }

    if (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::read must be called on Imagick instance");
    }

    std::string filename = args[1];

    if (!std::filesystem::exists(filename)) {
        throw std::invalid_argument("File does not exists: " + filename);
    }
    
    Magick::Image image;
    try {
        image.read(filename);
        
        // Validate the image is properly loaded
        size_t width = image.columns();
        size_t height = image.rows();
        
        if (width == 0 || height == 0) {
            throw std::runtime_error("Image loaded but has invalid dimensions (" +
                                    std::to_string(width) + "x" + std::to_string(height) + ")");
        }
        
        // Additional validation: check if image is valid
        if (!image.isValid()) {
            throw std::runtime_error("Image loaded but is marked as invalid by ImageMagick");
        }
        
        int handle = next_image_id++;
        images_[handle] = image;

        // Store the handle ON the object. Keying an external map by args[0].toString()
        // collided every instance, since a fresh Imagick serialises to the same string.
        Symbols::ValuePtr self = args[0];
        self->get<Symbols::ObjectMap>()["__image_id__"] = Symbols::ValuePtr(handle);
        return self;
    } catch (const Magick::Exception& e) {
        throw std::runtime_error("Failed to read image '" + filename + "': " + e.what());
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to read image '" + filename + "': " + e.what());
    }
}

Symbols::ValuePtr Modules::ImagickModule::crop(Symbols::FunctionArguments & args) {
    if (args.size() != 5) {
        throw std::invalid_argument(
            "Imagick::crop missing argument: (int width, int height, int xOffset, int , int yOffset)");
    }
    Magick::Image & image = imageFor(args, "crop");

    const int width   = args[1];
    const int height  = args[2];
    const int xOffset = args[3];
    const int yOffset = args[4];

    image.crop({ static_cast<size_t>(width), static_cast<size_t>(height), xOffset, yOffset });
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::resize(Symbols::FunctionArguments & args) {
    if (args.size() < 2) {
        throw std::invalid_argument(
            "Imagick::resize missing argument: (string $sizes | int $width, int $height, int $xOffset = 0, int $yOffset "
            "= 0)");
    }
    Magick::Image & image = imageFor(args, "resize");

    if (args[1] == Symbols::Variables::Type::STRING) {
        const std::string size = args[1];
        image.resize(size);
        return Symbols::ValuePtr::null();
    }

    int yOffset = 0;
    int xOffset = 0;
    if (args.size() < 3) {
        throw std::invalid_argument("Imagick::resize: Missing arguments");
    }
    if (args.size() == 5) {
        xOffset = args[3];
        yOffset = args[4];
    }

    const int width  = args[1];
    const int height = args[2];

    image.resize({ static_cast<size_t>(width), static_cast<size_t>(height), xOffset, yOffset });
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::write(Symbols::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::invalid_argument("Imagick::write missing argument: (string $filename)");
    }
    Magick::Image & image = imageFor(args, "write");
    const std::string filename = args[1];
    image.write(filename);

    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::mode(Symbols::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::invalid_argument("Imagick::mode missing argument: (string $mode)");
    }
    Magick::Image & image = imageFor(args, "mode");

    const std::string mode = args[1];
    if (mode == "RGB") {
        image.colorSpace(Magick::RGBColorspace);
    } else if (mode == "CMYK") {
        image.colorSpace(Magick::CMYKColorspace);
    } else if (mode == "GRAY") {
        image.colorSpace(Magick::GRAYColorspace);
    } else {
        throw std::invalid_argument("Imagick::mode: invalid mode. Supported modes are: RGB, CMYK, GRAY");
    }

    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::blur(Symbols::FunctionArguments & args) {
    if (args.size() != 3) {
        throw std::invalid_argument("Imagick::blur missing argument: (double radius, double sigma)");
    }
    Magick::Image & image = imageFor(args, "blur");

    const double radius = args[1];
    const double sigma  = args[2];
    image.blur(radius, sigma);

    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::rotate(Symbols::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::invalid_argument("Imagick::rotate missing argument: (double degrees)");
    }
    Magick::Image & image = imageFor(args, "rotate");

    const double degrees = args[1];
    image.rotate(degrees);

    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::flip(Symbols::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::invalid_argument("Imagick::flip missing argument: (string direction)");
    }
    Magick::Image & image = imageFor(args, "flip");

    const std::string direction = args[1];
    if (direction == "horizontal") {
        image.flip();
    } else if (direction == "vertical") {
        image.flop();
    } else {
        throw std::invalid_argument("Imagick::flip: invalid direction. Supported directions are: horizontal, vertical");
    }

    return Symbols::ValuePtr::null();
}


Magick::Image & Modules::ImagickModule::imageFor(Symbols::FunctionArguments & args, const char * method,
                                                 size_t argIndex) {
    const auto & objVal = args[argIndex];
    if (objVal != Symbols::Variables::Type::CLASS && objVal != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error(std::string("Imagick::") + method +
                                 (argIndex == 0 ? " must be called on an Imagick instance"
                                                : " expects an Imagick instance argument"));
    }
    const Symbols::ObjectMap & objMap = objVal->get<Symbols::ObjectMap>();
    auto                       idIt   = objMap.find("__image_id__");
    if (idIt == objMap.end() || idIt->second != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error(std::string("Imagick::") + method + ": no valid image - call read() first");
    }
    const int handle = idIt->second->get<int>();
    auto      imgIt  = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error(std::string("Imagick::") + method + ": image handle " +
                                 std::to_string(handle) + " is invalid");
    }
    return imgIt->second;
}

// getPixel(x, y) -> { int red, int green, int blue, int alpha }, each 0-255.
// Magick::Quantum depth varies by build (8 or 16 bit), so scale rather than assume.
Symbols::ValuePtr Modules::ImagickModule::getPixel(Symbols::FunctionArguments & args) {
    if (args.size() != 3 || args[1] != Symbols::Variables::Type::INTEGER ||
        args[2] != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::getPixel expects (int x, int y)");
    }
    Magick::Image & image = imageFor(args, "getPixel");

    const int x = args[1];
    const int y = args[2];
    if (x < 0 || y < 0 || static_cast<size_t>(x) >= image.columns() || static_cast<size_t>(y) >= image.rows()) {
        throw std::runtime_error("Imagick::getPixel: coordinates (" + std::to_string(x) + ", " +
                                 std::to_string(y) + ") are outside the image");
    }

    const Magick::ColorRGB colour(image.pixelColor(x, y));
    Symbols::ObjectMap     out;
    out["red"]   = Symbols::ValuePtr(static_cast<int>(colour.red() * 255.0 + 0.5));
    out["green"] = Symbols::ValuePtr(static_cast<int>(colour.green() * 255.0 + 0.5));
    out["blue"]  = Symbols::ValuePtr(static_cast<int>(colour.blue() * 255.0 + 0.5));
    out["alpha"] = Symbols::ValuePtr(static_cast<int>(colour.alpha() * 255.0 + 0.5));
    return Symbols::ValuePtr(out);
}

// setPixel(x, y, red, green, blue [, alpha]), each channel 0-255.
Symbols::ValuePtr Modules::ImagickModule::setPixel(Symbols::FunctionArguments & args) {
    if (args.size() < 6 || args.size() > 7) {
        throw std::runtime_error("Imagick::setPixel expects (int x, int y, int red, int green, int blue [, int alpha])");
    }
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] != Symbols::Variables::Type::INTEGER) {
            throw std::runtime_error("Imagick::setPixel expects integer arguments");
        }
    }
    Magick::Image & image = imageFor(args, "setPixel");

    const int x = args[1];
    const int y = args[2];
    if (x < 0 || y < 0 || static_cast<size_t>(x) >= image.columns() || static_cast<size_t>(y) >= image.rows()) {
        throw std::runtime_error("Imagick::setPixel: coordinates (" + std::to_string(x) + ", " +
                                 std::to_string(y) + ") are outside the image");
    }

    const auto channel = [](int v, const char * what) {
        if (v < 0 || v > 255) {
            throw std::runtime_error(std::string("Imagick::setPixel: ") + what + " must be 0-255");
        }
        return static_cast<double>(v) / 255.0;
    };

    // Writing needs the image to own its pixels outright, otherwise the change can be
    // lost to copy-on-write sharing with another Image referencing the same blob.
    image.modifyImage();

    Magick::ColorRGB colour(channel(args[3], "red"), channel(args[4], "green"), channel(args[5], "blue"));
    if (args.size() == 7) {
        const double a = channel(args[6], "alpha");
        // An image loaded from a format without an alpha channel has nowhere to store
        // one, and the write is silently discarded. Turn the channel on first.
        if (a < 1.0 && !image.alpha()) {
            image.alpha(true);
        }
        colour.alpha(a);
    } else {
        colour.alpha(1.0);
    }
    image.pixelColor(x, y, colour);

    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::getWidth(Symbols::FunctionArguments & args) {
    if (args.size() != 1) {
        throw std::invalid_argument("Imagick::getWidth takes no arguments");
    }
    Magick::Image & image = imageFor(args, "getWidth");
    return Symbols::ValuePtr(static_cast<int>(image.columns()));
}

Symbols::ValuePtr Modules::ImagickModule::getHeight(Symbols::FunctionArguments & args) {
    if (args.size() != 1) {
        throw std::invalid_argument("Imagick::getHeight takes no arguments");
    }
    Magick::Image & image = imageFor(args, "getHeight");

    return Symbols::ValuePtr(static_cast<int>(image.rows()));
}

Symbols::ValuePtr Modules::ImagickModule::composite(Symbols::FunctionArguments & args) {
    if (args.size() != 4) {
        throw std::invalid_argument("Imagick::composite missing arguments: (Imagick source, int x, int y)");
    }
    if (args[2] != Symbols::Variables::Type::INTEGER || args[3] != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::composite expects (Imagick source, int x, int y)");
    }
    Magick::Image & target = imageFor(args, "composite");
    Magick::Image & source = imageFor(args, "composite", 1);  // source is argument 1

    const int x = args[2];
    const int y = args[3];

    // The target owns its pixels before an in-place composite, otherwise the change can
    // be lost to copy-on-write sharing with another Image over the same blob.
    target.modifyImage();
    target.composite(source, x, y, Magick::OverCompositeOp);

    return Symbols::ValuePtr::null();
}
