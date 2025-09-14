// MemcachedModule.hpp
#ifndef MEMCACHEDMODULE_HPP
#define MEMCACHEDMODULE_HPP

#include <libmemcached/memcached.h>

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

class MemcachedClient {
private:
    memcached_st* memc = nullptr;
    bool connected = false;
    std::string servers;
    mutable std::mutex client_mutex;

public:
    MemcachedClient();
    ~MemcachedClient();

    // Connection management
    bool connect(const std::string& servers);
    void disconnect();
    bool isConnected() const { return connected; }

    // Core operations
    std::string get(const std::string& key);
    bool set(const std::string& key, const std::string& value, time_t expiration = 0);
    bool add(const std::string& key, const std::string& value, time_t expiration = 0);
    bool replace(const std::string& key, const std::string& value, time_t expiration = 0);
    bool delete_(const std::string& key);
    bool cas(const std::string& key, const std::string& value, time_t expiration, uint64_t cas_unique);

    // Numeric operations
    uint64_t incr(const std::string& key, uint64_t offset = 1);
    uint64_t decr(const std::string& key, uint64_t offset = 1);

    // Batch operations
    std::unordered_map<std::string, std::string> getMulti(const std::vector<std::string>& keys);
    bool setMulti(const std::unordered_map<std::string, std::string>& key_values, time_t expiration = 0);

    // Utility
    bool exists(const std::string& key);
    bool flush(time_t expiration = 0);
    std::string getError() const;

private:
    void cleanup();
    bool checkConnection();
};

// Connection wrapper for VoidScript OOP interface
class MemcachedConnectionWrapper {
private:
    static std::unordered_map<std::string, std::unique_ptr<MemcachedClient>> connection_map_;

public:
    static Symbols::ValuePtr construct(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr disconnect(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr isConnected(Symbols::FunctionArguments& args);

    // Core operations
    static Symbols::ValuePtr get(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr set(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr add(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr replace(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr delete_(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr cas(Symbols::FunctionArguments& args);

    // Numeric operations
    static Symbols::ValuePtr incr(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr decr(Symbols::FunctionArguments& args);

    // Batch operations
    static Symbols::ValuePtr getMulti(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr setMulti(Symbols::FunctionArguments& args);

    // Utility
    static Symbols::ValuePtr exists(Symbols::FunctionArguments& args);
    static Symbols::ValuePtr flush(Symbols::FunctionArguments& args);

private:
    static MemcachedClient* getClient(const std::string& objectId);
};

/**
 * Memcached Module providing both legacy functions and OOP interface
 */
class MemcachedModule final : public BaseModule {
public:
    MemcachedModule() {
        setModuleName("Memcached");
        setDescription("Provides memcached client functionality supporting all core operations: connect, get, set, delete, cas, incr, decr, and batch operations with configurable server connections.");
    }

    /**
     * @brief Register this module's symbols (legacy functions and OOP classes).
     */
    void registerFunctions() override;

    // Legacy functions for direct usage
    static Symbols::ValuePtr memcachedConnect(FunctionArguments& args);
    static Symbols::ValuePtr memcachedGet(FunctionArguments& args);
    static Symbols::ValuePtr memcachedSet(FunctionArguments& args);
    static Symbols::ValuePtr memcachedDelete(FunctionArguments& args);
    static Symbols::ValuePtr memcachedExists(FunctionArguments& args);
    static Symbols::ValuePtr memcachedFlush(FunctionArguments& args);
    static Symbols::ValuePtr memcachedIncr(FunctionArguments& args);
    static Symbols::ValuePtr memcachedDecr(FunctionArguments& args);

private:
    void registerLegacyFunctions();
    void registerOOPClasses();
};

} // namespace Modules

#endif // MEMCACHEDMODULE_HPP