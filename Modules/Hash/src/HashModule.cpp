// HashModule.cpp — OpenSSL-backed hash operations
#include "HashModule.hpp"

#include <openssl/evp.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

namespace {

std::string normalizeAlgo(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

const EVP_MD * lookupAlgo(const std::string & algorithm) {
    const std::string norm = normalizeAlgo(algorithm);
    const EVP_MD *    md   = EVP_get_digestbyname(norm.c_str());
    if (md == nullptr) {
        throw std::runtime_error("hash: unknown algorithm '" + algorithm +
                                 "'. Try sha256, sha512, sha1, md5, blake2b512.");
    }
    return md;
}

std::string toHex(const unsigned char * data, size_t n) {
    static const char hex[] = "0123456789abcdef";
    std::string       out;
    out.resize(n * 2);
    for (size_t i = 0; i < n; ++i) {
        out[i * 2]     = hex[data[i] >> 4];
        out[i * 2 + 1] = hex[data[i] & 0x0F];
    }
    return out;
}

struct EvpCtx {
    EVP_MD_CTX * ctx;
    EvpCtx() : ctx(EVP_MD_CTX_new()) {
        if (ctx == nullptr) {
            throw std::runtime_error("hash: EVP_MD_CTX_new failed");
        }
    }
    ~EvpCtx() {
        if (ctx != nullptr) {
            EVP_MD_CTX_free(ctx);
        }
    }
    EvpCtx(const EvpCtx &)             = delete;
    EvpCtx & operator=(const EvpCtx &) = delete;
};

std::string hashBytes(const std::string & algorithm, const void * data, size_t len) {
    const EVP_MD * md = lookupAlgo(algorithm);
    EvpCtx         c;
    if (EVP_DigestInit_ex(c.ctx, md, nullptr) != 1) {
        throw std::runtime_error("hash: EVP_DigestInit_ex failed");
    }
    if (EVP_DigestUpdate(c.ctx, data, len) != 1) {
        throw std::runtime_error("hash: EVP_DigestUpdate failed");
    }
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  digest_len = 0;
    if (EVP_DigestFinal_ex(c.ctx, digest, &digest_len) != 1) {
        throw std::runtime_error("hash: EVP_DigestFinal_ex failed");
    }
    return toHex(digest, digest_len);
}

std::string hashFileStreaming(const std::string & algorithm, const std::string & path) {
    const EVP_MD * md = lookupAlgo(algorithm);
    EvpCtx         c;
    if (EVP_DigestInit_ex(c.ctx, md, nullptr) != 1) {
        throw std::runtime_error("hash_file: EVP_DigestInit_ex failed");
    }

    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error("hash_file: cannot open: " + path);
    }
    std::vector<char> buf(64 * 1024);
    while (input.good()) {
        input.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        std::streamsize n = input.gcount();
        if (n <= 0) {
            break;
        }
        if (EVP_DigestUpdate(c.ctx, buf.data(), static_cast<size_t>(n)) != 1) {
            throw std::runtime_error("hash_file: EVP_DigestUpdate failed");
        }
    }
    if (input.bad()) {
        throw std::runtime_error("hash_file: I/O error reading: " + path);
    }
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  digest_len = 0;
    if (EVP_DigestFinal_ex(c.ctx, digest, &digest_len) != 1) {
        throw std::runtime_error("hash_file: EVP_DigestFinal_ex failed");
    }
    return toHex(digest, digest_len);
}

bool constantTimeEqual(const std::string & a, const std::string & b) {
    if (a.size() != b.size()) {
        return false;
    }
    unsigned char diff = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        diff |= static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]);
    }
    return diff == 0;
}

}  // namespace

void HashModule::registerFunctions() {
    std::vector<Symbols::FunctionParameterInfo> hash_string_params = {
        { "algorithm", Symbols::Variables::Type::STRING, "Hash algorithm name (e.g. \"sha256\")", false, false },
        { "data",      Symbols::Variables::Type::STRING, "Bytes to hash",                          false, false }
    };
    REGISTER_FUNCTION("hash_string", Symbols::Variables::Type::STRING, hash_string_params,
                      "Compute a hex digest of the given data using the named algorithm",
                      [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                          if (args.size() != 2 ||
                              args[0] != Symbols::Variables::Type::STRING ||
                              args[1] != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("hash_string expects (string algorithm, string data)");
                          }
                          const std::string algo = args[0]->get<std::string>();
                          const std::string data = args[1]->get<std::string>();
                          return Symbols::ValuePtr(hashBytes(algo, data.data(), data.size()));
                      });

    std::vector<Symbols::FunctionParameterInfo> hash_file_params = {
        { "algorithm", Symbols::Variables::Type::STRING, "Hash algorithm name", false, false },
        { "path",      Symbols::Variables::Type::STRING, "Path to file",        false, false }
    };
    REGISTER_FUNCTION("hash_file", Symbols::Variables::Type::STRING, hash_file_params,
                      "Stream a file through the named hash and return its hex digest",
                      [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                          if (args.size() != 2 ||
                              args[0] != Symbols::Variables::Type::STRING ||
                              args[1] != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("hash_file expects (string algorithm, string path)");
                          }
                          return Symbols::ValuePtr(hashFileStreaming(args[0]->get<std::string>(),
                                                                     args[1]->get<std::string>()));
                      });

    std::vector<Symbols::FunctionParameterInfo> compare_params = {
        { "a", Symbols::Variables::Type::STRING, "First string",  false, false },
        { "b", Symbols::Variables::Type::STRING, "Second string", false, false }
    };
    REGISTER_FUNCTION("hash_compare", Symbols::Variables::Type::BOOLEAN, compare_params,
                      "Constant-time equality check, intended for digest comparison",
                      [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                          if (args.size() != 2 ||
                              args[0] != Symbols::Variables::Type::STRING ||
                              args[1] != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("hash_compare expects (string a, string b)");
                          }
                          return Symbols::ValuePtr(constantTimeEqual(args[0]->get<std::string>(),
                                                                     args[1]->get<std::string>()));
                      });
}

}  // namespace Modules
