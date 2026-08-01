#include "ExifModule.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

void ExifModule::registerFunctions() {
    REGISTER_CLASS(this->name());
    using T = Symbols::Variables::Type;

    REGISTER_METHOD(this->name(), "__construct", {},
                    [this](FunctionArguments & args) { return this->construct(args); },
                    T::CLASS, "Create an Exif reader/editor");

    std::vector<Symbols::FunctionParameterInfo> path_param = { { "path", T::STRING, "Image file path" } };
    std::vector<Symbols::FunctionParameterInfo> key_param  = { { "key", T::STRING, "EXIF key, e.g. \"Exif.Image.Make\"" } };
    std::vector<Symbols::FunctionParameterInfo> kv_params  = { { "key", T::STRING, "EXIF key" },
                                                               { "value", T::STRING, "New value" } };

    REGISTER_METHOD(this->name(), "read", path_param,
                    [this](FunctionArguments & args) { return this->read(args); },
                    T::BOOLEAN, "Open an image and read its EXIF metadata");
    REGISTER_METHOD(this->name(), "getAll", {},
                    [this](FunctionArguments & args) { return this->getAll(args); },
                    T::OBJECT, "All EXIF tags as { key: value }");
    REGISTER_METHOD(this->name(), "get", key_param,
                    [this](FunctionArguments & args) { return this->get(args); },
                    T::STRING, "One EXIF tag's value (null if absent)");
    REGISTER_METHOD(this->name(), "set", kv_params,
                    [this](FunctionArguments & args) { return this->set(args); },
                    T::NULL_TYPE, "Set an EXIF tag (call save() to persist)");
    REGISTER_METHOD(this->name(), "remove", key_param,
                    [this](FunctionArguments & args) { return this->remove(args); },
                    T::BOOLEAN, "Remove one EXIF tag (returns whether it existed)");
    REGISTER_METHOD(this->name(), "clear", {},
                    [this](FunctionArguments & args) { return this->clear(args); },
                    T::NULL_TYPE, "Remove all EXIF tags");
    REGISTER_METHOD(this->name(), "count", {},
                    [this](FunctionArguments & args) { return this->count(args); },
                    T::INTEGER, "Number of EXIF tags currently held");
    REGISTER_METHOD(this->name(), "save", {},
                    [this](FunctionArguments & args) { return this->save(args); },
                    T::NULL_TYPE, "Write the EXIF changes back to the file in place");
    REGISTER_METHOD(this->name(), "saveAs", path_param,
                    [this](FunctionArguments & args) { return this->saveAs(args); },
                    T::NULL_TYPE, "Copy the image to a new path and write the current EXIF there");
}

Symbols::ValuePtr ExifModule::construct(FunctionArguments & args) {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error("Exif::__construct must be called on an Exif instance");
    }
    return args[0];
}

ExifModule::Entry & ExifModule::entryFor(FunctionArguments & args, const char * method) {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error(std::string("Exif::") + method + " must be called on an Exif instance");
    }
    const long id = Symbols::ValuePtr::instanceId(args[0]);
    auto       it = entries_.find(id);
    if (it == entries_.end() || !it->second.image) {
        throw std::runtime_error(std::string("Exif::") + method + ": no image loaded - call read() first");
    }
    return it->second;
}

Symbols::ValuePtr ExifModule::read(FunctionArguments & args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("Exif::read expects (string path)");
    }
    const long        id   = Symbols::ValuePtr::instanceId(args[0]);
    const std::string path = args[1]->get<std::string>();
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Exif::read: file does not exist: " + path);
    }
    try {
        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(path);
        image->readMetadata();
        Entry e;
        e.image       = std::move(image);
        e.path        = path;
        entries_[id]  = std::move(e);
    } catch (const Exiv2::Error & ex) {
        throw std::runtime_error(std::string("Exif::read failed: ") + ex.what());
    }
    return Symbols::ValuePtr(true);
}

Symbols::ValuePtr ExifModule::getAll(FunctionArguments & args) {
    Entry &                  e        = entryFor(args, "getAll");
    const Exiv2::ExifData &  exifData = e.image->exifData();
    Symbols::ObjectMap       out;
    for (auto it = exifData.begin(); it != exifData.end(); ++it) {
        out[it->key()] = Symbols::ValuePtr(it->toString());
    }
    return Symbols::ValuePtr(out);
}

Symbols::ValuePtr ExifModule::get(FunctionArguments & args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("Exif::get expects (string key)");
    }
    Entry &                 e   = entryFor(args, "get");
    const std::string       key = args[1]->get<std::string>();
    const Exiv2::ExifData & d   = e.image->exifData();
    auto                    it  = d.findKey(Exiv2::ExifKey(key));
    if (it == d.end()) {
        return Symbols::ValuePtr::null();
    }
    return Symbols::ValuePtr(it->toString());
}

Symbols::ValuePtr ExifModule::set(FunctionArguments & args) {
    if (args.size() != 3 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("Exif::set expects (string key, string value)");
    }
    Entry &           e     = entryFor(args, "set");
    const std::string key   = args[1]->get<std::string>();
    const std::string value = args[2]->toString();
    try {
        e.image->exifData()[key] = value;  // creates or overwrites the tag
    } catch (const Exiv2::Error & ex) {
        throw std::runtime_error(std::string("Exif::set failed for '") + key + "': " + ex.what());
    }
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr ExifModule::remove(FunctionArguments & args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("Exif::remove expects (string key)");
    }
    Entry &           e   = entryFor(args, "remove");
    const std::string key = args[1]->get<std::string>();
    Exiv2::ExifData & d   = e.image->exifData();
    auto              it  = d.findKey(Exiv2::ExifKey(key));
    if (it == d.end()) {
        return Symbols::ValuePtr(false);
    }
    d.erase(it);
    return Symbols::ValuePtr(true);
}

Symbols::ValuePtr ExifModule::clear(FunctionArguments & args) {
    Entry & e = entryFor(args, "clear");
    e.image->exifData().clear();
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr ExifModule::count(FunctionArguments & args) {
    Entry & e = entryFor(args, "count");
    return Symbols::ValuePtr(static_cast<int>(e.image->exifData().count()));
}

Symbols::ValuePtr ExifModule::save(FunctionArguments & args) {
    Entry & e = entryFor(args, "save");
    try {
        e.image->writeMetadata();
    } catch (const Exiv2::Error & ex) {
        throw std::runtime_error(std::string("Exif::save failed: ") + ex.what());
    }
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr ExifModule::saveAs(FunctionArguments & args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("Exif::saveAs expects (string path)");
    }
    Entry &           e    = entryFor(args, "saveAs");
    const std::string dest = args[1]->get<std::string>();
    try {
        std::filesystem::copy_file(e.path, dest, std::filesystem::copy_options::overwrite_existing);
        Exiv2::Image::UniquePtr out = Exiv2::ImageFactory::open(dest);
        out->readMetadata();
        out->setExifData(e.image->exifData());
        out->writeMetadata();
    } catch (const Exiv2::Error & ex) {
        throw std::runtime_error(std::string("Exif::saveAs failed: ") + ex.what());
    } catch (const std::filesystem::filesystem_error & ex) {
        throw std::runtime_error(std::string("Exif::saveAs: cannot copy to '") + dest + "': " + ex.what());
    }
    return Symbols::ValuePtr::null();
}

}  // namespace Modules
