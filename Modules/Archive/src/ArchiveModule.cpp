// ArchiveModule.cpp — libarchive-backed implementation.
#include "ArchiveModule.hpp"

#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

namespace {

namespace fs = std::filesystem;

int copyData(struct archive * ar, struct archive * aw) {
    const void * buff = nullptr;
    size_t       size = 0;
    la_int64_t   off  = 0;
    while (true) {
        int r = archive_read_data_block(ar, &buff, &size, &off);
        if (r == ARCHIVE_EOF) {
            return ARCHIVE_OK;
        }
        if (r < ARCHIVE_OK) {
            return r;
        }
        r = archive_write_data_block(aw, buff, size, off);
        if (r < ARCHIVE_OK) {
            return r;
        }
    }
}

// Strip up to `n` leading path components from a libarchive entry's pathname.
// Mirrors `tar --strip-components`: returns false if the entry has fewer
// components than `n` (caller should skip it in that case).
bool stripPathComponents(struct archive_entry * entry, int n) {
    if (n <= 0) {
        return true;
    }
    const char * orig = archive_entry_pathname(entry);
    if (orig == nullptr) {
        return false;
    }
    const std::string p(orig);
    int               stripped = 0;
    size_t            i        = 0;
    while (stripped < n && i < p.size()) {
        size_t slash = p.find('/', i);
        if (slash == std::string::npos) {
            return false;
        }
        i = slash + 1;
        ++stripped;
    }
    if (i >= p.size()) {
        return false;
    }
    archive_entry_set_pathname(entry, p.substr(i).c_str());
    return true;
}

const char * detectFormat(const std::string & format, int & filter, int & out_format) {
    filter     = ARCHIVE_FILTER_NONE;
    out_format = ARCHIVE_FORMAT_TAR_PAX_RESTRICTED;
    if (format == "tar.gz" || format == "tgz") {
        filter = ARCHIVE_FILTER_GZIP;
    } else if (format == "tar.xz" || format == "txz") {
        filter = ARCHIVE_FILTER_XZ;
    } else if (format == "tar.zst" || format == "tar.zstd" || format == "tzst") {
        filter = ARCHIVE_FILTER_ZSTD;
    } else if (format == "tar.bz2" || format == "tbz2") {
        filter = ARCHIVE_FILTER_BZIP2;
    } else if (format == "tar") {
        filter = ARCHIVE_FILTER_NONE;
    } else if (format == "zip") {
        out_format = ARCHIVE_FORMAT_ZIP;
        filter     = ARCHIVE_FILTER_NONE;
    } else {
        throw std::runtime_error("archive_create: unsupported format '" + format +
                                 "'. Try tar.gz, tar.xz, tar.zst, tar, zip");
    }
    return "ok";
}

std::string typeFromMode(mode_t m) {
    if (S_ISREG(m)) {
        return "file";
    }
    if (S_ISDIR(m)) {
        return "dir";
    }
    if (S_ISLNK(m)) {
        return "symlink";
    }
    return "other";
}

}  // namespace

static Symbols::ValuePtr archive_extract_fn(Symbols::FunctionArguments & args) {
    if (args.size() < 2 || args.size() > 3 ||
        args[0] != Symbols::Variables::Type::STRING ||
        args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("archive_extract expects (string archive_path, string dest_dir, object opts?)");
    }
    const std::string archive_path = args[0]->get<std::string>();
    const std::string dest_dir     = args[1]->get<std::string>();
    int               strip        = 0;
    if (args.size() == 3 && args[2] == Symbols::Variables::Type::OBJECT) {
        const auto & m  = args[2]->get<Symbols::ObjectMap>();
        auto         it = m.find("strip_components");
        if (it != m.end() && it->second == Symbols::Variables::Type::INTEGER) {
            strip = it->second->get<int>();
        }
    }

    std::error_code ec;
    fs::create_directories(dest_dir, ec);

    struct archive * a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    struct archive * ext = archive_write_disk_new();
    int              flags =
        ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS |
        ARCHIVE_EXTRACT_SECURE_NODOTDOT | ARCHIVE_EXTRACT_SECURE_SYMLINKS;
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);

    if (archive_read_open_filename(a, archive_path.c_str(), 10240) != ARCHIVE_OK) {
        std::string msg = archive_error_string(a) ? archive_error_string(a) : "open failed";
        archive_read_free(a);
        archive_write_free(ext);
        throw std::runtime_error("archive_extract: " + msg);
    }

    int r = ARCHIVE_OK;
    while (true) {
        struct archive_entry * entry = nullptr;
        r                            = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF) {
            break;
        }
        if (r < ARCHIVE_OK) {
            std::string msg = archive_error_string(a) ? archive_error_string(a) : "read_next_header";
            archive_read_free(a);
            archive_write_free(ext);
            throw std::runtime_error("archive_extract: " + msg);
        }

        if (!stripPathComponents(entry, strip)) {
            continue;  // not enough components or empty after stripping
        }

        // prepend dest_dir
        const char * p          = archive_entry_pathname(entry);
        std::string  full_path  = dest_dir + "/" + (p ? p : "");
        archive_entry_set_pathname(entry, full_path.c_str());

        r = archive_write_header(ext, entry);
        if (r < ARCHIVE_OK) {
            std::string msg = archive_error_string(ext) ? archive_error_string(ext) : "write_header";
            archive_read_free(a);
            archive_write_free(ext);
            throw std::runtime_error("archive_extract: " + msg);
        }
        if (archive_entry_size(entry) > 0) {
            r = copyData(a, ext);
            if (r < ARCHIVE_OK) {
                std::string msg = archive_error_string(ext) ? archive_error_string(ext) : "copy_data";
                archive_read_free(a);
                archive_write_free(ext);
                throw std::runtime_error("archive_extract: " + msg);
            }
        }
        archive_write_finish_entry(ext);
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
    return Symbols::ValuePtr::null();
}

static Symbols::ValuePtr archive_create_fn(Symbols::FunctionArguments & args) {
    if (args.size() < 3 || args.size() > 4 ||
        args[0] != Symbols::Variables::Type::STRING ||
        args[1] != Symbols::Variables::Type::STRING ||
        args[2] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error(
            "archive_create expects (string out_path, string format, array entries, object opts?)");
    }
    const std::string out_path = args[0]->get<std::string>();
    const std::string format   = args[1]->get<std::string>();
    bool              deterministic = true;
    if (args.size() == 4 && args[3] == Symbols::Variables::Type::OBJECT) {
        const auto & m  = args[3]->get<Symbols::ObjectMap>();
        auto         it = m.find("deterministic");
        if (it != m.end() && it->second == Symbols::Variables::Type::BOOLEAN) {
            deterministic = it->second->get<bool>();
        }
    }

    int filter      = 0;
    int out_fmt     = 0;
    detectFormat(format, filter, out_fmt);

    struct archive * w = archive_write_new();
    if (out_fmt == ARCHIVE_FORMAT_ZIP) {
        archive_write_set_format_zip(w);
    } else {
        archive_write_set_format_pax_restricted(w);
    }
    if (filter == ARCHIVE_FILTER_GZIP) {
        archive_write_add_filter_gzip(w);
    } else if (filter == ARCHIVE_FILTER_XZ) {
        archive_write_add_filter_xz(w);
    } else if (filter == ARCHIVE_FILTER_BZIP2) {
        archive_write_add_filter_bzip2(w);
    } else if (filter == ARCHIVE_FILTER_ZSTD) {
        archive_write_add_filter_zstd(w);
    }

    if (archive_write_open_filename(w, out_path.c_str()) != ARCHIVE_OK) {
        std::string msg = archive_error_string(w) ? archive_error_string(w) : "open_filename";
        archive_write_free(w);
        throw std::runtime_error("archive_create: " + msg);
    }

    const auto & entry_map = args[2]->get<Symbols::ObjectMap>();
    std::vector<char> buf(64 * 1024);
    for (const auto & kv : entry_map) {
        if (kv.second != Symbols::Variables::Type::OBJECT) {
            archive_write_close(w);
            archive_write_free(w);
            throw std::runtime_error("archive_create: entry '" + kv.first + "' must be an object");
        }
        const auto & em = kv.second->get<Symbols::ObjectMap>();
        auto         sit = em.find("source");
        auto         dit = em.find("dest");
        if (sit == em.end() || dit == em.end() ||
            sit->second != Symbols::Variables::Type::STRING ||
            dit->second != Symbols::Variables::Type::STRING) {
            archive_write_close(w);
            archive_write_free(w);
            throw std::runtime_error("archive_create: entry '" + kv.first +
                                     "' must have string 'source' and 'dest'");
        }
        const std::string source = sit->second->get<std::string>();
        const std::string dest   = dit->second->get<std::string>();
        int               mode_override = -1;
        auto              mit  = em.find("mode");
        if (mit != em.end() && mit->second == Symbols::Variables::Type::INTEGER) {
            mode_override = mit->second->get<int>();
        }

        struct stat st;
        if (::lstat(source.c_str(), &st) != 0) {
            archive_write_close(w);
            archive_write_free(w);
            throw std::runtime_error("archive_create: cannot stat: " + source);
        }

        struct archive_entry * entry = archive_entry_new();
        archive_entry_set_pathname(entry, dest.c_str());
        archive_entry_set_size(entry, S_ISREG(st.st_mode) ? st.st_size : 0);
        archive_entry_set_filetype(entry,
                                   S_ISREG(st.st_mode) ? AE_IFREG :
                                   S_ISDIR(st.st_mode) ? AE_IFDIR :
                                   S_ISLNK(st.st_mode) ? AE_IFLNK : AE_IFREG);
        archive_entry_set_perm(entry, mode_override >= 0 ? static_cast<mode_t>(mode_override)
                                                         : (st.st_mode & 07777));
        if (deterministic) {
            archive_entry_set_mtime(entry, 0, 0);
            archive_entry_set_uid(entry, 0);
            archive_entry_set_gid(entry, 0);
            archive_entry_set_uname(entry, "");
            archive_entry_set_gname(entry, "");
        } else {
            archive_entry_set_mtime(entry, st.st_mtime, 0);
            archive_entry_set_uid(entry, st.st_uid);
            archive_entry_set_gid(entry, st.st_gid);
        }

        if (S_ISLNK(st.st_mode)) {
            char linkbuf[4096];
            ssize_t n = ::readlink(source.c_str(), linkbuf, sizeof(linkbuf) - 1);
            if (n > 0) {
                linkbuf[n] = '\0';
                archive_entry_set_symlink(entry, linkbuf);
            }
        }

        if (archive_write_header(w, entry) != ARCHIVE_OK) {
            std::string msg = archive_error_string(w) ? archive_error_string(w) : "write_header";
            archive_entry_free(entry);
            archive_write_close(w);
            archive_write_free(w);
            throw std::runtime_error("archive_create: " + msg);
        }

        if (S_ISREG(st.st_mode)) {
            std::ifstream in(source, std::ios::in | std::ios::binary);
            if (!in.is_open()) {
                archive_entry_free(entry);
                archive_write_close(w);
                archive_write_free(w);
                throw std::runtime_error("archive_create: cannot open source: " + source);
            }
            while (in.good()) {
                in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
                std::streamsize n = in.gcount();
                if (n <= 0) {
                    break;
                }
                la_ssize_t wn = archive_write_data(w, buf.data(), static_cast<size_t>(n));
                if (wn < 0) {
                    std::string msg = archive_error_string(w) ? archive_error_string(w) : "write_data";
                    archive_entry_free(entry);
                    archive_write_close(w);
                    archive_write_free(w);
                    throw std::runtime_error("archive_create: " + msg);
                }
            }
        }
        archive_entry_free(entry);
    }

    archive_write_close(w);
    archive_write_free(w);
    return Symbols::ValuePtr::null();
}

static Symbols::ValuePtr archive_list_fn(Symbols::FunctionArguments & args) {
    if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("archive_list expects one string archive path");
    }
    const std::string archive_path = args[0]->get<std::string>();

    struct archive * a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    if (archive_read_open_filename(a, archive_path.c_str(), 10240) != ARCHIVE_OK) {
        std::string msg = archive_error_string(a) ? archive_error_string(a) : "open failed";
        archive_read_free(a);
        throw std::runtime_error("archive_list: " + msg);
    }

    Symbols::ObjectMap out;
    size_t             idx = 0;
    while (true) {
        struct archive_entry * entry = nullptr;
        int                    r     = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF) {
            break;
        }
        if (r < ARCHIVE_OK) {
            std::string msg = archive_error_string(a) ? archive_error_string(a) : "read_next_header";
            archive_read_free(a);
            throw std::runtime_error("archive_list: " + msg);
        }

        Symbols::ObjectMap e;
        e["path"]  = Symbols::ValuePtr(std::string(archive_entry_pathname(entry) ? archive_entry_pathname(entry) : ""));
        int sz_val = static_cast<int>(archive_entry_size(entry));
        e["size"]  = Symbols::ValuePtr(sz_val);
        int md_val = static_cast<int>(archive_entry_perm(entry));
        e["mode"]  = Symbols::ValuePtr(md_val);
        int mt_val = static_cast<int>(archive_entry_mtime(entry));
        e["mtime"] = Symbols::ValuePtr(mt_val);
        e["type"]  = Symbols::ValuePtr(typeFromMode(archive_entry_filetype(entry)));
        out[std::to_string(idx++)] = Symbols::ValuePtr(e);

        archive_read_data_skip(a);
    }
    archive_read_close(a);
    archive_read_free(a);
    return Symbols::ValuePtr(out);
}

void ArchiveModule::registerFunctions() {
    std::vector<Symbols::FunctionParameterInfo> extract_params = {
        { "archive_path", Symbols::Variables::Type::STRING, "Path to the archive file",                            false, false },
        { "dest_dir",     Symbols::Variables::Type::STRING, "Directory to extract into (created if it doesn't exist)", false, false },
        { "options",      Symbols::Variables::Type::OBJECT, "Optional: { strip_components: int }",                 true,  false }
    };
    REGISTER_FUNCTION("archive_extract", Symbols::Variables::Type::NULL_TYPE, extract_params,
                      "Extract any libarchive-supported archive (tar.gz/xz/zst, zip, ...) into dest_dir",
                      &archive_extract_fn);

    std::vector<Symbols::FunctionParameterInfo> create_params = {
        { "out_path", Symbols::Variables::Type::STRING, "Path to the archive to create",                                 false, false },
        { "format",   Symbols::Variables::Type::STRING, "tar.gz | tar.xz | tar.zst | tar.bz2 | tar | zip",               false, false },
        { "entries",  Symbols::Variables::Type::OBJECT, "Object/array of { source, dest, mode? } entry descriptors",     false, false },
        { "options",  Symbols::Variables::Type::OBJECT, "Optional: { deterministic: bool (default true) }",              true,  false }
    };
    REGISTER_FUNCTION("archive_create", Symbols::Variables::Type::NULL_TYPE, create_params,
                      "Create an archive from a list of source files",
                      &archive_create_fn);

    std::vector<Symbols::FunctionParameterInfo> list_params = {
        { "archive_path", Symbols::Variables::Type::STRING, "Path to the archive file", false, false }
    };
    REGISTER_FUNCTION("archive_list", Symbols::Variables::Type::OBJECT, list_params,
                      "List entries in an archive without extracting. "
                      "Returns object<int, { path, size, mode, mtime, type }>",
                      &archive_list_fn);
}

}  // namespace Modules
