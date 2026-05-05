// ArchiveModule.hpp
#ifndef ARCHIVEMODULE_HPP
#define ARCHIVEMODULE_HPP

#include "Modules/BaseModule.hpp"

namespace Modules {

/**
 * @brief Archive extraction/creation/listing backed by libarchive.
 *
 * archive_extract(archive_path, dest_dir, opts?)
 *   opts: { strip_components: int }
 *
 * archive_create(out_path, format, entries, opts?)
 *   format:  "tar.gz" | "tar.xz" | "tar.zst" | "tar" | "zip"
 *   entries: object<string,object> with each value { source: string, dest: string, mode?: int }
 *   opts:    { deterministic: bool, default true }
 *
 * archive_list(archive_path)  -> object<string,object>
 *   each value:  { path, size, mode, mtime, type ("file"|"dir"|"symlink"|"other") }
 */
class ArchiveModule : public BaseModule {
  public:
    ArchiveModule() {
        setModuleName("Archive");
        setDescription("Archive extraction, creation, and listing via libarchive "
                       "(supports tar.{gz,xz,zst}, zip, and others libarchive understands)");
        setBuiltIn(false);
    }

    void registerFunctions() override;
};

}  // namespace Modules

#endif  // ARCHIVEMODULE_HPP
