// HashModule.cpp — OpenSSL-backed hash operations
#include "HashModule.hpp"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

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

// Keyed HMAC over `msg`, returned as a hex digest (reuses lookupAlgo/toHex).
std::string hmacHex(const std::string & algorithm, const std::string & key, const std::string & msg) {
    const EVP_MD * md  = lookupAlgo(algorithm);
    unsigned char  out[EVP_MAX_MD_SIZE];
    unsigned int   len = 0;
    if (HMAC(md, key.data(), static_cast<int>(key.size()),
             reinterpret_cast<const unsigned char *>(msg.data()), msg.size(), out, &len) == nullptr) {
        throw std::runtime_error("hmac: computation failed");
    }
    return toHex(out, len);
}

// n cryptographically-secure random bytes (raw; hex_encode() for a hex token).
std::string randomBytes(int n) {
    if (n < 0 || n > (1 << 20)) {
        throw std::runtime_error("random_bytes: count must be 0-1048576");
    }
    std::string buf(static_cast<size_t>(n), '\0');
    if (n > 0 && RAND_bytes(reinterpret_cast<unsigned char *>(&buf[0]), n) != 1) {
        throw std::runtime_error("random_bytes: RAND_bytes failed");
    }
    return buf;
}

// --- base32 (RFC 4648) ---------------------------------------------------------------

std::string base32Encode(const std::string & in) {
    static const char * alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    std::string         out;
    int                 bits = 0;
    unsigned long       val  = 0;
    for (unsigned char c : in) {
        val = (val << 8) | c;
        bits += 8;
        while (bits >= 5) {
            out.push_back(alphabet[(val >> (bits - 5)) & 0x1F]);
            bits -= 5;
        }
    }
    if (bits > 0) {
        out.push_back(alphabet[(val << (5 - bits)) & 0x1F]);
    }
    while (out.size() % 8 != 0) {
        out.push_back('=');
    }
    return out;
}

std::string base32Decode(const std::string & in) {
    int           bits = 0;
    unsigned long val  = 0;
    std::string   out;
    for (char ch : in) {
        if (ch == '=' || std::isspace(static_cast<unsigned char>(ch))) {
            continue;
        }
        int d;
        if (ch >= 'A' && ch <= 'Z') {
            d = ch - 'A';
        } else if (ch >= 'a' && ch <= 'z') {
            d = ch - 'a';
        } else if (ch >= '2' && ch <= '7') {
            d = ch - '2' + 26;
        } else {
            throw std::runtime_error("base32_decode: invalid character");
        }
        val = (val << 5) | static_cast<unsigned long>(d);
        bits += 5;
        if (bits >= 8) {
            out.push_back(static_cast<char>((val >> (bits - 8)) & 0xFF));
            bits -= 8;
        }
    }
    return out;
}

// --- AES-256-GCM ---------------------------------------------------------------------
// The passphrase is turned into a 32-byte key with SHA-256. The output blob is
// iv(12) || tag(16) || ciphertext (raw bytes; hex_encode() for storage/transport).

std::string deriveKey32(const std::string & key) {
    unsigned char md[32];
    unsigned int  len = 0;
    if (EVP_Digest(key.data(), key.size(), md, &len, EVP_sha256(), nullptr) != 1) {
        throw std::runtime_error("aes: key derivation failed");
    }
    return std::string(reinterpret_cast<char *>(md), 32);
}

std::string aesEncrypt(const std::string & key, const std::string & plain) {
    const std::string k = deriveKey32(key);
    unsigned char     iv[12];
    if (RAND_bytes(iv, sizeof(iv)) != 1) {
        throw std::runtime_error("aes_encrypt: RAND_bytes failed");
    }
    EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("aes_encrypt: context alloc failed");
    }
    std::string out(plain.size(), '\0');
    unsigned char tag[16];
    int           outlen = 0;
    int           total  = 0;
    bool          ok =
        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) == 1 &&
        EVP_EncryptInit_ex(ctx, nullptr, nullptr, reinterpret_cast<const unsigned char *>(k.data()), iv) == 1 &&
        EVP_EncryptUpdate(ctx, plain.empty() ? nullptr : reinterpret_cast<unsigned char *>(&out[0]), &outlen,
                          reinterpret_cast<const unsigned char *>(plain.data()),
                          static_cast<int>(plain.size())) == 1;
    total = outlen;
    ok    = ok && EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char *>(&out[0]) + total, &outlen) == 1;
    total += outlen;
    ok = ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag) == 1;
    EVP_CIPHER_CTX_free(ctx);
    if (!ok) {
        throw std::runtime_error("aes_encrypt: encryption failed");
    }
    out.resize(total);
    std::string blob;
    blob.append(reinterpret_cast<char *>(iv), 12);
    blob.append(reinterpret_cast<char *>(tag), 16);
    blob.append(out);
    return blob;
}

std::string aesDecrypt(const std::string & key, const std::string & blob) {
    if (blob.size() < 28) {
        throw std::runtime_error("aes_decrypt: input too short (need iv+tag+ciphertext)");
    }
    const std::string    k   = deriveKey32(key);
    const unsigned char * iv  = reinterpret_cast<const unsigned char *>(blob.data());
    const unsigned char * tag = iv + 12;
    const char *          ct  = blob.data() + 28;
    const int             ctlen = static_cast<int>(blob.size()) - 28;

    EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("aes_decrypt: context alloc failed");
    }
    std::string out(static_cast<size_t>(ctlen), '\0');
    int         outlen = 0;
    int         total  = 0;
    bool        ok =
        EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) == 1 &&
        EVP_DecryptInit_ex(ctx, nullptr, nullptr, reinterpret_cast<const unsigned char *>(k.data()), iv) == 1 &&
        EVP_DecryptUpdate(ctx, ctlen == 0 ? nullptr : reinterpret_cast<unsigned char *>(&out[0]), &outlen,
                          reinterpret_cast<const unsigned char *>(ct), ctlen) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, const_cast<unsigned char *>(tag)) == 1;
    total       = outlen;
    const int rc = ok ? EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char *>(&out[0]) + total, &outlen) : 0;
    EVP_CIPHER_CTX_free(ctx);
    if (!ok || rc <= 0) {
        throw std::runtime_error("aes_decrypt: authentication failed (wrong key or corrupted data)");
    }
    total += outlen;
    out.resize(total);
    return out;
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

    std::vector<Symbols::FunctionParameterInfo> hmac_params = {
        { "algorithm", Symbols::Variables::Type::STRING, "Hash algorithm name (e.g. \"sha256\")", false, false },
        { "key",       Symbols::Variables::Type::STRING, "Secret key",                             false, false },
        { "data",      Symbols::Variables::Type::STRING, "Message to authenticate",                false, false }
    };
    REGISTER_FUNCTION("hmac", Symbols::Variables::Type::STRING, hmac_params,
                      "Keyed HMAC of the data under the named algorithm; returns a hex digest",
                      [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                          if (args.size() != 3 ||
                              args[0] != Symbols::Variables::Type::STRING ||
                              args[1] != Symbols::Variables::Type::STRING ||
                              args[2] != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("hmac expects (string algorithm, string key, string data)");
                          }
                          return Symbols::ValuePtr(hmacHex(args[0]->get<std::string>(),
                                                           args[1]->get<std::string>(),
                                                           args[2]->get<std::string>()));
                      });

    std::vector<Symbols::FunctionParameterInfo> rand_params = {
        { "count", Symbols::Variables::Type::INTEGER, "Number of random bytes", false, false }
    };
    REGISTER_FUNCTION("random_bytes", Symbols::Variables::Type::STRING, rand_params,
                      "Return count cryptographically-secure random bytes (use hex_encode() for a token)",
                      [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                          if (args.size() != 1 || args[0] != Symbols::Variables::Type::INTEGER) {
                              throw std::runtime_error("random_bytes expects (int count)");
                          }
                          return Symbols::ValuePtr(randomBytes(args[0]->get<int>()));
                      });

    std::vector<Symbols::FunctionParameterInfo> one_string = {
        { "value", Symbols::Variables::Type::STRING, "Input string", false, false }
    };
    REGISTER_FUNCTION("base32_encode", Symbols::Variables::Type::STRING, one_string,
                      "Base32-encode a string (RFC 4648, e.g. for TOTP secrets)",
                      [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                          if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("base32_encode expects one string");
                          }
                          return Symbols::ValuePtr(base32Encode(args[0]->get<std::string>()));
                      });
    REGISTER_FUNCTION("base32_decode", Symbols::Variables::Type::STRING, one_string,
                      "Decode a Base32 string back to bytes",
                      [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                          if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("base32_decode expects one string");
                          }
                          return Symbols::ValuePtr(base32Decode(args[0]->get<std::string>()));
                      });

    std::vector<Symbols::FunctionParameterInfo> aes_params = {
        { "key",  Symbols::Variables::Type::STRING, "Passphrase (hashed to a 256-bit key)", false, false },
        { "data", Symbols::Variables::Type::STRING, "Plaintext / ciphertext blob",          false, false }
    };
    REGISTER_FUNCTION("aes_encrypt", Symbols::Variables::Type::STRING, aes_params,
                      "AES-256-GCM encrypt; returns iv+tag+ciphertext bytes (hex_encode() to store)",
                      [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                          if (args.size() != 2 || args[0] != Symbols::Variables::Type::STRING ||
                              args[1] != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("aes_encrypt expects (string key, string plaintext)");
                          }
                          return Symbols::ValuePtr(aesEncrypt(args[0]->get<std::string>(), args[1]->get<std::string>()));
                      });
    REGISTER_FUNCTION("aes_decrypt", Symbols::Variables::Type::STRING, aes_params,
                      "AES-256-GCM decrypt an iv+tag+ciphertext blob; throws if authentication fails",
                      [](Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                          if (args.size() != 2 || args[0] != Symbols::Variables::Type::STRING ||
                              args[1] != Symbols::Variables::Type::STRING) {
                              throw std::runtime_error("aes_decrypt expects (string key, string blob)");
                          }
                          return Symbols::ValuePtr(aesDecrypt(args[0]->get<std::string>(), args[1]->get<std::string>()));
                      });
}

}  // namespace Modules
