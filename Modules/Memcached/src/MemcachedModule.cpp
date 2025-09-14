// MemcachedModule implementation: Memcached client using libmemcached
#include "MemcachedModule.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <sstream>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

// Static member definitions
std::unordered_map<std::string, std::unique_ptr<MemcachedClient>> MemcachedConnectionWrapper::connection_map_;

// MemcachedClient implementation
MemcachedClient::MemcachedClient() : memc(nullptr), connected(false), servers("") {
}

MemcachedClient::~MemcachedClient() {
    cleanup();
}

bool MemcachedClient::connect(const std::string& servers) {
    std::lock_guard<std::mutex> lock(client_mutex);

    if (connected) {
        disconnect();
    }

    this->servers = servers;
    memc = memcached_create(nullptr);

    if (!memc) {
        return false;
    }

    memcached_return_t rc = memcached_server_add(memc, servers.c_str(), 11211);
    if (rc != MEMCACHED_SUCCESS) {
        cleanup();
        return false;
    }

    // Set behavior options
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);

    connected = true;
    return true;
}

void MemcachedClient::disconnect() {
    std::lock_guard<std::mutex> lock(client_mutex);
    cleanup();
    connected = false;
}

bool MemcachedClient::checkConnection() {
    if (!connected || !memc) {
        return false;
    }

    // Simple ping to check if server is responsive
    return true; // For now, just check internal state
}

std::string MemcachedClient::get(const std::string& key) {
    if (!checkConnection()) {
        throw std::runtime_error("Memcached client not connected");
    }

    memcached_return_t rc;
    size_t value_length;
    uint32_t flags;

    char* value = memcached_get(memc, key.c_str(), key.length(),
                               &value_length, &flags, &rc);

    if (rc == MEMCACHED_SUCCESS && value) {
        std::string result(value, value_length);
        free(value);
        return result;
    }

    if (value) free(value);

    if (rc == MEMCACHED_NOTFOUND) {
        throw std::runtime_error("Key not found");
    }

    throw std::runtime_error("Memcached get failed: " + std::string(memcached_strerror(memc, rc)));
}

bool MemcachedClient::set(const std::string& key, const std::string& value, time_t expiration) {
    if (!checkConnection()) {
        throw std::runtime_error("Memcached client not connected");
    }

    memcached_return_t rc = memcached_set(memc, key.c_str(), key.length(),
                                         value.c_str(), value.length(),
                                         expiration, 0);

    if (rc == MEMCACHED_SUCCESS) {
        return true;
    }

    throw std::runtime_error("Memcached set failed: " + std::string(memcached_strerror(memc, rc)));
}

bool MemcachedClient::add(const std::string& key, const std::string& value, time_t expiration) {
    if (!checkConnection()) {
        throw std::runtime_error("Memcached client not connected");
    }

    memcached_return_t rc = memcached_add(memc, key.c_str(), key.length(),
                                         value.c_str(), value.length(),
                                         expiration, 0);

    return rc == MEMCACHED_SUCCESS;
}

bool MemcachedClient::replace(const std::string& key, const std::string& value, time_t expiration) {
    if (!checkConnection()) {
        throw std::runtime_error("Memcached client not connected");
    }

    memcached_return_t rc = memcached_replace(memc, key.c_str(), key.length(),
                                             value.c_str(), value.length(),
                                             expiration, 0);

    return rc == MEMCACHED_SUCCESS;
}

bool MemcachedClient::delete_(const std::string& key) {
    if (!checkConnection()) {
        throw std::runtime_error("Memcached client not connected");
    }

    memcached_return_t rc = memcached_delete(memc, key.c_str(), key.length(), 0);

    return rc == MEMCACHED_SUCCESS || rc == MEMCACHED_NOTFOUND;
}

bool MemcachedClient::cas(const std::string& key, const std::string& value,
                          time_t expiration, uint64_t cas_unique) {
    if (!checkConnection()) {
        throw std::runtime_error("Memcached client not connected");
    }

    memcached_return_t rc = memcached_cas(memc, key.c_str(), key.length(),
                                         value.c_str(), value.length(),
                                         expiration, 0, cas_unique);

    return rc == MEMCACHED_SUCCESS;
}

uint64_t MemcachedClient::incr(const std::string& key, uint64_t offset) {
    if (!checkConnection()) {
        throw std::runtime_error("Memcached client not connected");
    }

    uint64_t value = 0;
    memcached_return_t rc = memcached_increment(memc, key.c_str(), key.length(),
                                               offset, &value);

    if (rc == MEMCACHED_SUCCESS) {
        return value;
    }

    throw std::runtime_error("Memcached incr failed: " + std::string(memcached_strerror(memc, rc)));
}

uint64_t MemcachedClient::decr(const std::string& key, uint64_t offset) {
    if (!checkConnection()) {
        throw std::runtime_error("Memcached client not connected");
    }

    uint64_t value = 0;
    memcached_return_t rc = memcached_decrement(memc, key.c_str(), key.length(),
                                               offset, &value);

    if (rc == MEMCACHED_SUCCESS) {
        return value;
    }

    throw std::runtime_error("Memcached decr failed: " + std::string(memcached_strerror(memc, rc)));
}

std::unordered_map<std::string, std::string> MemcachedClient::getMulti(const std::vector<std::string>& keys) {
    if (!checkConnection()) {
        throw std::runtime_error("Memcached client not connected");
    }

    std::unordered_map<std::string, std::string> result_map;

    // Use memcached_mget to request the keys
    if (keys.empty()) {
        return result_map;
    }

    // Convert string vector to C style string array
    std::vector<const char*> c_keys;
    std::vector<size_t> key_lengths;
    for (const auto& key : keys) {
        c_keys.push_back(key.c_str());
        key_lengths.push_back(key.length());
    }

    memcached_return_t rc = memcached_mget(memc, c_keys.data(), key_lengths.data(), keys.size());
    if (rc != MEMCACHED_SUCCESS) {
        throw std::runtime_error("Memcached mget failed: " + std::string(memcached_strerror(memc, rc)));
    }

    // Fetch results
    memcached_result_st* result = memcached_result_create(memc, nullptr);
    if (!result) {
        throw std::runtime_error("Failed to create memcached result structure");
    }

    // Keep track of which keys we've found
    std::unordered_map<std::string, int> requested_keys;
    for (size_t i = 0; i < keys.size(); ++i) {
        requested_keys[keys[i]] = i;
    }

    while ((result = memcached_fetch_result(memc, result, &rc))) {
        if (rc == MEMCACHED_SUCCESS) {
            const char* key = memcached_result_key_value(result);
            const char* value = memcached_result_value(result);
            size_t value_length = memcached_result_length(result);

            if (key && value && value_length > 0 && requested_keys.find(key) != requested_keys.end()) {
                result_map[key] = std::string(value, value_length);
            }
        }
    }

    memcached_result_free(result);
    return result_map;
}

bool MemcachedClient::setMulti(const std::unordered_map<std::string, std::string>& key_values, time_t expiration) {
    if (!checkConnection()) {
        throw std::runtime_error("Memcached client not connected");
    }

    // Multi-set operation - for simplicity, we'll set each key individually
    // In a more sophisticated implementation, we could use the bulk API
    for (const auto& kv : key_values) {
        if (!set(kv.first, kv.second, expiration)) {
            return false;
        }
    }

    return true;
}

bool MemcachedClient::exists(const std::string& key) {
    try {
        get(key);
        return true;
    } catch (const std::runtime_error&) {
        return false;
    }
}

bool MemcachedClient::flush(time_t expiration) {
    if (!checkConnection()) {
        throw std::runtime_error("Memcached client not connected");
    }

    memcached_return_t rc = memcached_flush(memc, expiration);
    return rc == MEMCACHED_SUCCESS;
}

std::string MemcachedClient::getError() const {
    if (!memc) {
        return "Memcached client not initialized";
    }

    memcached_return_t rc = memcached_last_error(memc);
    if (rc == MEMCACHED_SUCCESS) {
        return "";
    }

    return memcached_strerror(memc, rc);
}

void MemcachedClient::cleanup() {
    if (memc) {
        memcached_free(memc);
        memc = nullptr;
    }
    connected = false;
}

// MemcachedConnectionWrapper implementation
Symbols::ValuePtr MemcachedConnectionWrapper::construct(Symbols::FunctionArguments& args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("MemcachedConnection::construct expects servers string");
    }

    std::string objectId = args[0].toString();
    std::string servers = args[1].get<std::string>();

    connection_map_[objectId] = std::make_unique<MemcachedClient>();

    if (!connection_map_[objectId]->connect(servers)) {
        throw std::runtime_error("Failed to connect to memcached servers: " + servers);
    }

    return args[0];
}

Symbols::ValuePtr MemcachedConnectionWrapper::disconnect(Symbols::FunctionArguments& args) {
    std::string objectId = args[0].toString();
    auto it = connection_map_.find(objectId);
    if (it != connection_map_.end()) {
        it->second->disconnect();
    }
    return Symbols::ValuePtr("");
}

Symbols::ValuePtr MemcachedConnectionWrapper::isConnected(Symbols::FunctionArguments& args) {
    std::string objectId = args[0].toString();
    auto it = connection_map_.find(objectId);
    if (it != connection_map_.end()) {
        return Symbols::ValuePtr(it->second->isConnected());
    }
    return Symbols::ValuePtr(false);
}

Symbols::ValuePtr MemcachedConnectionWrapper::get(Symbols::FunctionArguments& args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("MemcachedConnection::get expects key string");
    }

    std::string objectId = args[0].toString();
    std::string key = args[1].get<std::string>();

    MemcachedClient* client = getClient(objectId);
    return Symbols::ValuePtr(client->get(key));
}

Symbols::ValuePtr MemcachedConnectionWrapper::set(Symbols::FunctionArguments& args) {
    if (args.size() < 3 || args.size() > 4 ||
        args[1] != Symbols::Variables::Type::STRING ||
        args[2] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("MemcachedConnection::set expects key, value, and optional expiration");
    }

    std::string objectId = args[0].toString();
    std::string key = args[1].get<std::string>();
    std::string value = args[2].get<std::string>();
    time_t expiration = 0;

    if (args.size() == 4 && args[3] == Symbols::Variables::Type::INTEGER) {
        expiration = args[3].get<int>();
    }

    MemcachedClient* client = getClient(objectId);
    return Symbols::ValuePtr(client->set(key, value, expiration));
}

Symbols::ValuePtr MemcachedConnectionWrapper::add(Symbols::FunctionArguments& args) {
    if (args.size() < 3 || args.size() > 4 ||
        args[1] != Symbols::Variables::Type::STRING ||
        args[2] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("MemcachedConnection::add expects key, value, and optional expiration");
    }

    std::string objectId = args[0].toString();
    std::string key = args[1].get<std::string>();
    std::string value = args[2].get<std::string>();
    time_t expiration = 0;

    if (args.size() == 4 && args[3] == Symbols::Variables::Type::INTEGER) {
        expiration = args[3].get<int>();
    }

    MemcachedClient* client = getClient(objectId);
    return Symbols::ValuePtr(client->add(key, value, expiration));
}

Symbols::ValuePtr MemcachedConnectionWrapper::replace(Symbols::FunctionArguments& args) {
    if (args.size() < 3 || args.size() > 4 ||
        args[1] != Symbols::Variables::Type::STRING ||
        args[2] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("MemcachedConnection::replace expects key, value, and optional expiration");
    }

    std::string objectId = args[0].toString();
    std::string key = args[1].get<std::string>();
    std::string value = args[2].get<std::string>();
    time_t expiration = 0;

    if (args.size() == 4 && args[3] == Symbols::Variables::Type::INTEGER) {
        expiration = args[3].get<int>();
    }

    MemcachedClient* client = getClient(objectId);
    return Symbols::ValuePtr(client->replace(key, value, expiration));
}

Symbols::ValuePtr MemcachedConnectionWrapper::delete_(Symbols::FunctionArguments& args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("MemcachedConnection::delete expects key string");
    }

    std::string objectId = args[0].toString();
    std::string key = args[1].get<std::string>();

    MemcachedClient* client = getClient(objectId);
    return Symbols::ValuePtr(client->delete_(key));
}

Symbols::ValuePtr MemcachedConnectionWrapper::cas(Symbols::FunctionArguments& args) {
    if (args.size() != 4 ||
        args[1] != Symbols::Variables::Type::STRING ||
        args[2] != Symbols::Variables::Type::STRING ||
        args[3] == Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("MemcachedConnection::cas expects key, value, expiration, and cas_unique");
    }

    std::string objectId = args[0].toString();
    std::string key = args[1].get<std::string>();
    std::string value = args[2].get<std::string>();
    time_t expiration = args[3].get<int>();
    uint64_t cas_unique = args[4].get<int>();

    MemcachedClient* client = getClient(objectId);
    return Symbols::ValuePtr(client->cas(key, value, expiration, cas_unique));
}

Symbols::ValuePtr MemcachedConnectionWrapper::incr(Symbols::FunctionArguments& args) {
    if (args.size() < 2 || args.size() > 3 ||
        args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("MemcachedConnection::incr expects key and optional offset");
    }

    std::string objectId = args[0].toString();
    std::string key = args[1].get<std::string>();
    uint64_t offset = 1;

    if (args.size() == 3 && args[2] == Symbols::Variables::Type::INTEGER) {
        offset = args[2].get<int>();
    }

    MemcachedClient* client = getClient(objectId);
    return Symbols::ValuePtr((int)client->incr(key, offset));
}

Symbols::ValuePtr MemcachedConnectionWrapper::decr(Symbols::FunctionArguments& args) {
    if (args.size() < 2 || args.size() > 3 ||
        args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("MemcachedConnection::decr expects key and optional offset");
    }

    std::string objectId = args[0].toString();
    std::string key = args[1].get<std::string>();
    uint64_t offset = 1;

    if (args.size() == 3 && args[2] == Symbols::Variables::Type::INTEGER) {
        offset = args[2].get<int>();
    }

    MemcachedClient* client = getClient(objectId);
    return Symbols::ValuePtr((int)client->decr(key, offset));
}

Symbols::ValuePtr MemcachedConnectionWrapper::getMulti(Symbols::FunctionArguments& args) {
    // Implementation for getMulti
    return Symbols::ValuePtr("");
}

Symbols::ValuePtr MemcachedConnectionWrapper::setMulti(Symbols::FunctionArguments& args) {
    // Implementation for setMulti
    return Symbols::ValuePtr("");
}

Symbols::ValuePtr MemcachedConnectionWrapper::exists(Symbols::FunctionArguments& args) {
    // Implementation for exists
    return Symbols::ValuePtr("");
}

Symbols::ValuePtr MemcachedConnectionWrapper::flush(Symbols::FunctionArguments& args) {
    // Implementation for flush
    return Symbols::ValuePtr("");
}

MemcachedClient* MemcachedConnectionWrapper::getClient(const std::string& objectId) {
    auto it = connection_map_.find(objectId);
    if (it == connection_map_.end()) {
        throw std::runtime_error("MemcachedConnection client not found for object: " + objectId);
    }
    return it->second.get();
}

// MemcachedModule implementation
void MemcachedModule::registerFunctions() {
    registerLegacyFunctions();
    registerOOPClasses();
}

void MemcachedModule::registerLegacyFunctions() {
    // Connection function
    std::vector<Symbols::FunctionParameterInfo> connect_param = {
        {"servers", Symbols::Variables::Type::STRING, "Memcached servers string (e.g., 'localhost')"}
    };
    REGISTER_FUNCTION("memcachedConnect", Symbols::Variables::Type::STRING, connect_param,
                      "Connect to memcached servers",
                      [this](const FunctionArguments & args) -> Symbols::ValuePtr {
                          return this->memcachedConnect(args);
                      });

    // Basic operations
    std::vector<Symbols::FunctionParameterInfo> key_param = {
        {"key", Symbols::Variables::Type::STRING, "Cache key"}
    };
    REGISTER_FUNCTION("memcachedGet", Symbols::Variables::Type::STRING, key_param,
                      "Get value from memcached",
                      [this](const FunctionArguments & args) -> Symbols::ValuePtr {
                          return this->memcachedGet(args);
                      });

    std::vector<Symbols::FunctionParameterInfo> set_params = {
        {"key", Symbols::Variables::Type::STRING, "Cache key"},
        {"value", Symbols::Variables::Type::STRING, "Value to store"},
        {"expiration", Symbols::Variables::Type::INTEGER, "Expiration time in seconds", true}
    };
    REGISTER_FUNCTION("memcachedSet", Symbols::Variables::Type::BOOLEAN, set_params,
                      "Set value in memcached",
                      [this](const FunctionArguments & args) -> Symbols::ValuePtr {
                          return this->memcachedSet(args);
                      });

    REGISTER_FUNCTION("memcachedDelete", Symbols::Variables::Type::BOOLEAN, key_param,
                      "Delete key from memcached",
                      [this](const FunctionArguments & args) -> Symbols::ValuePtr {
                          return this->memcachedDelete(args);
                      });

    REGISTER_FUNCTION("memcachedExists", Symbols::Variables::Type::BOOLEAN, key_param,
                      "Check if key exists in memcached",
                      [this](const FunctionArguments & args) -> Symbols::ValuePtr {
                          return this->memcachedExists(args);
                      });

    REGISTER_FUNCTION("memcachedFlush", Symbols::Variables::Type::BOOLEAN, {},
                      "Flush all cache entries",
                      [this](const FunctionArguments & args) -> Symbols::ValuePtr {
                          return this->memcachedFlush(args);
                      });

    std::vector<Symbols::FunctionParameterInfo> incr_params = {
        {"key", Symbols::Variables::Type::STRING, "Cache key"},
        {"offset", Symbols::Variables::Type::INTEGER, "Increment offset", true}
    };
    REGISTER_FUNCTION("memcachedIncr", Symbols::Variables::Type::INTEGER, incr_params,
                      "Increment numeric value",
                      [this](const FunctionArguments & args) -> Symbols::ValuePtr {
                          return this->memcachedIncr(args);
                      });

    REGISTER_FUNCTION("memcachedDecr", Symbols::Variables::Type::INTEGER, incr_params,
                      "Decrement numeric value",
                      [this](const FunctionArguments & args) -> Symbols::ValuePtr {
                          return this->memcachedDecr(args);
                      });
}

void MemcachedModule::registerOOPClasses() {
    // Register MemcachedConnection class
    REGISTER_CLASS("MemcachedConnection");

    std::vector<Symbols::FunctionParameterInfo> servers_param = {
        {"servers", Symbols::Variables::Type::STRING, "Memcached servers"}
    };
    REGISTER_METHOD("MemcachedConnection", "construct", servers_param,
                    MemcachedConnectionWrapper::construct,
                    Symbols::Variables::Type::CLASS, "Create MemcachedConnection");

    // Add other methods...
    std::vector<Symbols::FunctionParameterInfo> no_params = {};
    REGISTER_METHOD("MemcachedConnection", "disconnect", no_params,
                    MemcachedConnectionWrapper::disconnect,
                    Symbols::Variables::Type::STRING, "Disconnect from memcached");

    REGISTER_METHOD("MemcachedConnection", "isConnected", no_params,
                    MemcachedConnectionWrapper::isConnected,
                    Symbols::Variables::Type::BOOLEAN, "Check connection status");

    std::vector<Symbols::FunctionParameterInfo> key_param = {
        {"key", Symbols::Variables::Type::STRING, "Cache key"}
    };
    REGISTER_METHOD("MemcachedConnection", "get", key_param,
                    MemcachedConnectionWrapper::get,
                    Symbols::Variables::Type::STRING, "Get cached value");

    std::vector<Symbols::FunctionParameterInfo> set_params = {
        {"key", Symbols::Variables::Type::STRING, "Cache key"},
        {"value", Symbols::Variables::Type::STRING, "Value to store"},
        {"expiration", Symbols::Variables::Type::INTEGER, "Expiration time", true}
    };
    REGISTER_METHOD("MemcachedConnection", "set", set_params,
                    MemcachedConnectionWrapper::set,
                    Symbols::Variables::Type::BOOLEAN, "Set cached value");

    REGISTER_METHOD("MemcachedConnection", "delete", key_param,
                    MemcachedConnectionWrapper::delete_,
                    Symbols::Variables::Type::BOOLEAN, "Delete cached key");
}

// Legacy function implementations
Symbols::ValuePtr MemcachedModule::memcachedConnect(FunctionArguments& args) {
    if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("memcachedConnect expects servers string");
    }

    const std::string& servers = args[0].get<std::string>();
    // For simplicity, create a temporary client
    MemcachedClient client;
    if (client.connect(servers)) {
        return Symbols::ValuePtr("connected");
    }
    throw std::runtime_error("Failed to connect to memcached servers");
}

Symbols::ValuePtr MemcachedModule::memcachedGet(FunctionArguments& args) {
    if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("memcachedGet expects key string");
    }

    // For legacy functions, we'd need a global connection or session management
    // This is a simplified implementation
    throw std::runtime_error("memcachedGet: No active connection. Use OOP interface instead.");
}

Symbols::ValuePtr MemcachedModule::memcachedSet(FunctionArguments& args) {
    if (args.size() < 2 || args.size() > 3 ||
        args[0] != Symbols::Variables::Type::STRING ||
        args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("memcachedSet expects key, value, and optional expiration");
    }

    throw std::runtime_error("memcachedSet: No active connection. Use OOP interface instead.");
}

Symbols::ValuePtr MemcachedModule::memcachedDelete(FunctionArguments& args) {
    if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("memcachedDelete expects key string");
    }

    throw std::runtime_error("memcachedDelete: No active connection. Use OOP interface instead.");
}

Symbols::ValuePtr MemcachedModule::memcachedExists(FunctionArguments& args) {
    if (args.size() != 1 || args[0] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("memcachedExists expects key string");
    }

    throw std::runtime_error("memcachedExists: No active connection. Use OOP interface instead.");
}

Symbols::ValuePtr MemcachedModule::memcachedFlush(FunctionArguments& args) {
    throw std::runtime_error("memcachedFlush: No active connection. Use OOP interface instead.");
}

Symbols::ValuePtr MemcachedModule::memcachedIncr(FunctionArguments& args) {
    if (args.size() < 1 || args.size() > 2 || args[0] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("memcachedIncr expects key and optional offset");
    }

    throw std::runtime_error("memcachedIncr: No active connection. Use OOP interface instead.");
}

Symbols::ValuePtr MemcachedModule::memcachedDecr(FunctionArguments& args) {
    if (args.size() < 1 || args.size() > 2 || args[0] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("memcachedDecr expects key and optional offset");
    }

    throw std::runtime_error("memcachedDecr: No active connection. Use OOP interface instead.");
}

}  // namespace Modules