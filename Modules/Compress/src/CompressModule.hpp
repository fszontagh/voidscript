#ifndef COMPRESS_MODULE_HPP
#define COMPRESS_MODULE_HPP

#include "Modules/BaseModule.hpp"

namespace Modules {

// gzencode(data [, level]) -> gzip bytes;  gzdecode(data) -> original bytes.
class CompressModule : public BaseModule {
  public:
    CompressModule() {
        setModuleName("Compress");
        setDescription("gzip compression: gzencode / gzdecode, backed by zlib");
    }
    void registerFunctions() override;
};

}  // namespace Modules
#endif  // COMPRESS_MODULE_HPP
