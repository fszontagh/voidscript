#include "ImagickModule.hpp"

#include <filesystem>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/Value.hpp"

// Define the static member
std::unordered_map<std::string, int> Modules::ImagickModule::object_to_handle_map_;

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

    //params = {
    //    { "mode", Symbols::Variables::Type::STRING },
    //};

    //    REGISTER_METHOD(
    //        this->name(), "mode",params, [this](const FunctionArguments & args) { return this->mode(args); },
    //        Symbols::Variables::Type::NULL_TYPE, "Change image mode");

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
    //REGISTER_METHOD(
    //    this->name(), "composite", [this](const FunctionArguments & args) { return this->composite(args); },
    //    Symbols::Variables::Type::NULL_TYPE, "Composite image");
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
        
        // NEW APPROACH: Use object's toString as unique identifier
        std::string objectId = args[0].toString();
        object_to_handle_map_[objectId] = handle;
        
        // Still return the original object, but now we track state externally
        return args[0];
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
    const auto & objVal = args[0];
    if (objVal != Symbols::Variables::Type::CLASS && objVal != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::crop must be called on Imagick instance");
    }
    Symbols::ObjectMap objMap   = objVal;
    int                handle   = 0;
    auto               itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::crop: no valid image");
    }

    handle     = itHandle->second;
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::crop: image not found");
    }

    const int width   = args[1];
    const int height  = args[2];
    const int xOffset = args[3];
    const int yOffset = args[4];

    imgIt->second.crop({ static_cast<size_t>(width), static_cast<size_t>(height), xOffset, yOffset });
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::resize(Symbols::FunctionArguments & args) {
    if (args.size() < 2) {
        throw std::invalid_argument(
            "Imagick::resize missing argument: (string $sizes | int $width, int $height, int $xOffset = 0, int $yOffset "
            "= 0)");
    }
    const auto & objVal = args[0];
    if (objVal != Symbols::Variables::Type::CLASS && objVal != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::resize must be called on Imagick instance");
    }
    
    // NEW APPROACH: Look up handle by object identity
    std::string objectId = args[0].toString();
    
    auto handleIt = object_to_handle_map_.find(objectId);
    if (handleIt == object_to_handle_map_.end()) {
        throw std::runtime_error("Imagick::resize: no valid image");
    }

    int handle = handleIt->second;
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::resize: image not found");
    }

    if (args[1] == Symbols::Variables::Type::STRING) {
        const std::string size = args[1];
        imgIt->second.resize(size);
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

    imgIt->second.resize({ static_cast<size_t>(width), static_cast<size_t>(height), xOffset, yOffset });
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::write(Symbols::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::invalid_argument("Imagick::write missing argument: (string $filename)");
    }
    const auto & objVal = args[0];
    if (objVal != Symbols::Variables::Type::CLASS && objVal != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::write must be called on Imagick instance");
    }
    
    // NEW APPROACH: Look up handle by object identity
    std::string objectId = args[0].toString();
    
    auto handleIt = object_to_handle_map_.find(objectId);
    if (handleIt == object_to_handle_map_.end()) {
        throw std::runtime_error("Imagick::write: no valid image");
    }

    int handle = handleIt->second;
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::write: image not found");
    }
    const std::string filename = args[1];
    imgIt->second.write(filename);

    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::mode(Symbols::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::invalid_argument("Imagick::mode missing argument: (string $mode)");
    }
    const auto & objVal = args[0];
    if (objVal != Symbols::Variables::Type::CLASS && objVal != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::mode must be called on Imagick instance");
    }
    Symbols::ObjectMap objMap   = objVal;
    int                handle   = 0;
    auto               itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::mode: no valid image");
    }

    handle     = itHandle->second;
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::mode: image not found");
    }

    const std::string mode = args[1];
    if (mode == "RGB") {
        imgIt->second.colorSpace(Magick::RGBColorspace);
    } else if (mode == "CMYK") {
        imgIt->second.colorSpace(Magick::CMYKColorspace);
    } else if (mode == "GRAY") {
        imgIt->second.colorSpace(Magick::GRAYColorspace);
    } else {
        throw std::invalid_argument("Imagick::mode: invalid mode. Supported modes are: RGB, CMYK, GRAY");
    }

    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::blur(Symbols::FunctionArguments & args) {
    if (args.size() != 3) {
        throw std::invalid_argument("Imagick::blur missing argument: (double radius, double sigma)");
    }
    const auto & objVal = args[0];
    if (objVal != Symbols::Variables::Type::CLASS && objVal != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::blur must be called on Imagick instance");
    }
    Symbols::ObjectMap objMap   = objVal;
    int                handle   = 0;
    auto               itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::blur: no valid image");
    }

    handle     = itHandle->second;
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::blur: image not found");
    }

    const double radius = args[1];
    const double sigma  = args[2];
    imgIt->second.blur(radius, sigma);

    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::rotate(Symbols::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::invalid_argument("Imagick::rotate missing argument: (double degrees)");
    }
    const auto & objVal = args[0];
    if (objVal != Symbols::Variables::Type::CLASS && objVal != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::rotate must be called on Imagick instance");
    }
    Symbols::ObjectMap objMap = objVal;

    int  handle   = 0;
    auto itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::rotate: no valid image");
    }

    handle     = itHandle->second;
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::rotate: image not found");
    }

    const double degrees = args[1];
    imgIt->second.rotate(degrees);

    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::flip(Symbols::FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::invalid_argument("Imagick::flip missing argument: (string direction)");
    }
    const auto & objVal = args[0];
    if (objVal != Symbols::Variables::Type::CLASS && objVal != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::flip must be called on Imagick instance");
    }
    Symbols::ObjectMap objMap   = objVal;
    int                handle   = 0;
    auto               itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::flip: no valid image");
    }

    handle     = itHandle->second;
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::flip: image not found");
    }

    const std::string direction = args[1];
    if (direction == "horizontal") {
        imgIt->second.flip();
    } else if (direction == "vertical") {
        imgIt->second.flop();
    } else {
        throw std::invalid_argument("Imagick::flip: invalid direction. Supported directions are: horizontal, vertical");
    }

    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr Modules::ImagickModule::getWidth(Symbols::FunctionArguments & args) {
    if (args.size() != 1) {
        throw std::invalid_argument("Imagick::getWidth takes no arguments");
    }
    const auto & objVal = args[0];
    if (objVal != Symbols::Variables::Type::CLASS && objVal != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::getWidth must be called on Imagick instance");
    }
    
    // NEW APPROACH: Look up handle by object identity
    std::string objectId = args[0].toString();
    
    auto handleIt = object_to_handle_map_.find(objectId);
    if (handleIt == object_to_handle_map_.end()) {
        throw std::runtime_error("Imagick::getWidth: no valid image - object was not properly initialized by read()");
    }
    
    int handle = handleIt->second;
    
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::getWidth: image not found - handle " + std::to_string(handle) + " is invalid");
    }

    return Symbols::ValuePtr(static_cast<int>(imgIt->second.columns()));
}

Symbols::ValuePtr Modules::ImagickModule::getHeight(Symbols::FunctionArguments & args) {
    if (args.size() != 1) {
        throw std::invalid_argument("Imagick::getHeight takes no arguments");
    }
    const auto & objVal = args[0];
    if (objVal != Symbols::Variables::Type::CLASS && objVal != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::getHeight must be called on Imagick instance");
    }
    
    // NEW APPROACH: Look up handle by object identity
    std::string objectId = args[0].toString();
    
    auto handleIt = object_to_handle_map_.find(objectId);
    if (handleIt == object_to_handle_map_.end()) {
        throw std::runtime_error("Imagick::getHeight: no valid image - object was not properly initialized by read()");
    }
    
    int handle = handleIt->second;
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::getHeight: image not found - handle " + std::to_string(handle) + " is invalid");
    }

    return Symbols::ValuePtr(static_cast<int>(imgIt->second.rows()));
}

Symbols::ValuePtr Modules::ImagickModule::composite(Symbols::FunctionArguments & args) {
    if (args.size() != 4) {
        throw std::invalid_argument("Imagick::composite missing arguments: (Imagick source, int x, int y)");
    }
    const auto & objVal = args[0];
    if (objVal != Symbols::Variables::Type::CLASS && objVal != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::composite must be called on Imagick instance");
    }
    Symbols::ObjectMap objMap   = objVal;
    int                handle   = 0;
    auto               itHandle = objMap.find("__image_id__");
    if (itHandle == objMap.end() || itHandle->second != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::composite: no valid image");
    }

    handle     = itHandle->second;
    auto imgIt = images_.find(handle);
    if (imgIt == images_.end()) {
        throw std::runtime_error("Imagick::composite: image not found");
    }

    // Forrás kép lekérése
    const auto & sourceVal = args[1];
    if (sourceVal != Symbols::Variables::Type::CLASS && sourceVal != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("Imagick::composite: source must be an Imagick instance");
    }
    Symbols::ObjectMap sourceMap    = sourceVal;
    auto               sourceHandle = sourceMap.find("__image_id__");
    if (sourceHandle == sourceMap.end() || sourceHandle->second != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Imagick::composite: no valid source image");
    }

    int  sourceImageHandle = sourceHandle->second;
    auto sourceImgIt       = images_.find(sourceImageHandle);
    if (sourceImgIt == images_.end()) {
        throw std::runtime_error("Imagick::composite: source image not found");
    }

    const int x = args[2];
    const int y = args[3];

    imgIt->second.composite(sourceImgIt->second, x, y, Magick::OverCompositeOp);

    return Symbols::ValuePtr::null();
}
