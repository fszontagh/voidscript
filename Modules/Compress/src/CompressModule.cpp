#include "CompressModule.hpp"

#include <zlib.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

namespace {

std::string gzencode(const std::string & in, int level) {
    z_stream zs{};
    // windowBits 15 | 16 selects the gzip wrapper (rather than raw zlib).
    if (deflateInit2(&zs, level, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        throw std::runtime_error("gzencode: deflateInit2 failed");
    }
    zs.next_in  = reinterpret_cast<Bytef *>(const_cast<char *>(in.data()));
    zs.avail_in = static_cast<uInt>(in.size());

    std::string out;
    char        buf[16384];
    int         ret;
    do {
        zs.next_out  = reinterpret_cast<Bytef *>(buf);
        zs.avail_out = sizeof(buf);
        ret          = deflate(&zs, Z_FINISH);
        out.append(buf, sizeof(buf) - zs.avail_out);
    } while (ret == Z_OK);
    deflateEnd(&zs);
    if (ret != Z_STREAM_END) {
        throw std::runtime_error("gzencode: compression failed");
    }
    return out;
}

std::string gzdecode(const std::string & in) {
    z_stream zs{};
    // windowBits 15 | 32 auto-detects a gzip or zlib header.
    if (inflateInit2(&zs, 15 | 32) != Z_OK) {
        throw std::runtime_error("gzdecode: inflateInit2 failed");
    }
    zs.next_in  = reinterpret_cast<Bytef *>(const_cast<char *>(in.data()));
    zs.avail_in = static_cast<uInt>(in.size());

    std::string out;
    char        buf[16384];
    int         ret;
    do {
        zs.next_out  = reinterpret_cast<Bytef *>(buf);
        zs.avail_out = sizeof(buf);
        ret          = inflate(&zs, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&zs);
            throw std::runtime_error("gzdecode: invalid or corrupt gzip data");
        }
        out.append(buf, sizeof(buf) - zs.avail_out);
    } while (ret != Z_STREAM_END);
    inflateEnd(&zs);
    return out;
}

}  // namespace

void CompressModule::registerFunctions() {
    std::vector<Symbols::FunctionParameterInfo> enc_params = {
        { "data",  Symbols::Variables::Type::STRING,  "Bytes to compress" },
        { "level", Symbols::Variables::Type::INTEGER, "Compression level 0-9 (default 6)", true }
    };
    REGISTER_FUNCTION("gzencode", Symbols::Variables::Type::STRING, enc_params,
                      "gzip-compress a string (optional level 0-9, default 6)",
                      [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                          if (args.empty() || args[0] != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("gzencode expects (string data [, int level])");
                          }
                          int level = 6;
                          if (args.size() >= 2 && args[1] == Symbols::Variables::Type::INTEGER) {
                              level = args[1]->get<int>();
                              if (level < 0 || level > 9) {
                                  throw std::runtime_error("gzencode: level must be 0-9");
                              }
                          }
                          return Symbols::ValuePtr(gzencode(args[0]->get<std::string>(), level));
                      });

    std::vector<Symbols::FunctionParameterInfo> dec_params = {
        { "data", Symbols::Variables::Type::STRING, "gzip (or zlib) bytes to decompress" }
    };
    REGISTER_FUNCTION("gzdecode", Symbols::Variables::Type::STRING, dec_params,
                      "Decompress gzip (or zlib) data back to the original bytes",
                      [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                          if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("gzdecode expects (string data)");
                          }
                          return Symbols::ValuePtr(gzdecode(args[0]->get<std::string>()));
                      });
}

}  // namespace Modules
