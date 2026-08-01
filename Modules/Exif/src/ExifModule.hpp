#ifndef EXIF_MODULE_HPP
#define EXIF_MODULE_HPP

#include <exiv2/exiv2.hpp>

#include <memory>
#include <string>
#include <unordered_map>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

// VoidScript class "Exif": read and edit EXIF metadata via exiv2.
//   Exif $e = new Exif();  $e->read("photo.jpg");
//   auto $all = $e->getAll();               // { "Exif.Image.Make": "Canon", ... }
//   $e->set("Exif.Image.Copyright", "me");  $e->remove("Exif.Photo.UserComment");
//   $e->save();                             // write back in place (or saveAs(path))
class ExifModule : public BaseModule {
  public:
    ExifModule() {
        setModuleName("Exif");
        setDescription("Read and edit EXIF metadata (get/getAll/set/remove/clear/save/saveAs) via exiv2");
    }
    void registerFunctions() override;

  private:
    struct Entry {
        // Concrete std::unique_ptr rather than Exiv2::Image::UniquePtr: the alias name
        // varies across exiv2 versions (AutoPtr in 0.27, UniquePtr in newer 0.28), but
        // both are std::unique_ptr<Image>, which is what ImageFactory::open() returns.
        std::unique_ptr<Exiv2::Image> image;
        std::string                   path;
    };
    std::unordered_map<long, Entry> entries_;

    Entry &           entryFor(FunctionArguments & args, const char * method);
    Symbols::ValuePtr construct(FunctionArguments & args);
    Symbols::ValuePtr read(FunctionArguments & args);
    Symbols::ValuePtr getAll(FunctionArguments & args);
    Symbols::ValuePtr get(FunctionArguments & args);
    Symbols::ValuePtr set(FunctionArguments & args);
    Symbols::ValuePtr remove(FunctionArguments & args);
    Symbols::ValuePtr clear(FunctionArguments & args);
    Symbols::ValuePtr count(FunctionArguments & args);
    Symbols::ValuePtr save(FunctionArguments & args);
    Symbols::ValuePtr saveAs(FunctionArguments & args);
};

}  // namespace Modules
#endif  // EXIF_MODULE_HPP
