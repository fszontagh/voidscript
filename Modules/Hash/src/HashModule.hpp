// HashModule.hpp
#ifndef HASHMODULE_HPP
#define HASHMODULE_HPP

#include <string>

#include "Modules/BaseModule.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

/**
 * @brief Cryptographic hash module backed by OpenSSL EVP.
 *
 * hash_string(algorithm, data)   -> string (hex digest)
 * hash_file(algorithm, path)     -> string (hex digest, streamed)
 * hash_compare(a, b)             -> bool   (constant-time string compare)
 *
 * Supported algorithms (case-insensitive):
 *   "sha256", "sha512", "sha1", "md5", "blake2b512", "blake2s256"
 * The set is whatever the linked OpenSSL provides via EVP_get_digestbyname.
 */
class HashModule : public BaseModule {
  public:
    HashModule() {
        setModuleName("Hash");
        setDescription("Cryptographic hashes (SHA-256/512, BLAKE2, MD5, SHA-1) "
                       "for strings or files, plus a constant-time compare.");
        setBuiltIn(false);
    }

    void registerFunctions() override;
};

}  // namespace Modules

#endif  // HASHMODULE_HPP
