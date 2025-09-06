// MongoDBModule.hpp
#ifndef MONGODBMODULE_MONGODBMODULE_HPP
#define MONGODBMODULE_MONGODBMODULE_HPP

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/collection.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/types.hpp>

#include <memory>
#include <string>
#include <mutex>
#include <chrono>
#include <map>

namespace Modules {

/**
 * @brief Custom exception hierarchy for MongoDB operations
 */
class MongoDBException : public Modules::Exception {
private:
    int error_code_;

public:
    MongoDBException(const std::string& message, int error_code = 0)
        : Exception(message), error_code_(error_code) {}

    int getErrorCode() const { return error_code_; }
};

/**
 * @brief Connection-specific exceptions
 */
class ConnectionException : public MongoDBException {
public:
    ConnectionException(const std::string& message, int error_code = 0)
        : MongoDBException(message, error_code) {}
};

/**
 * @brief Query-specific exceptions
 */
class QueryException : public MongoDBException {
public:
    QueryException(const std::string& message, int error_code = 0)
        : MongoDBException(message, error_code) {}
};

/**
 * @brief Connection configuration structure
 */
struct ConnectionConfig {
    std::string uri;
    std::string database;

    // Validation method
    bool isValid() const {
        return !uri.empty() && !database.empty();
    }
};

/**
 * @brief Database connection wrapper
 */
class DatabaseConnection {
private:
    mongocxx::client client_;
    mongocxx::database database_;
    ConnectionConfig config_;
    bool is_connected_;

public:
    // Constructor/destructor
    explicit DatabaseConnection(const ConnectionConfig& config);
    ~DatabaseConnection();

    // Non-copyable but movable
    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;
    DatabaseConnection(DatabaseConnection&&) noexcept;
    DatabaseConnection& operator=(DatabaseConnection&&) noexcept;

    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const { return is_connected_; }

    // Accessors
    mongocxx::client& getClient() { return client_; }
    mongocxx::database& getDatabase() { return database_; }
    const ConnectionConfig& getConfig() const { return config_; }

private:
    void cleanup();
};

/**
 * @brief Document utilities for BSON handling and VoidScript type conversion
 */
class Document {
public:
    // VoidScript conversion
    static Symbols::ValuePtr toVoidScriptObject(const bsoncxx::document::view& bson_view);
    static bsoncxx::document::value fromVoidScriptObject(const Symbols::ValuePtr& obj);

    // BSON conversion helpers
    static bsoncxx::v_noabi::types::bson_value::value convertToBSONValue(const Symbols::ValuePtr& value);
    static Symbols::ValuePtr convertFromBSONValue(const bsoncxx::v_noabi::types::bson_value::view& bson_value);
};

/**
 * @brief Simplified MongoDB module main class - only basic operations
 */
class MongoDBModule : public BaseModule {
private:
    std::unique_ptr<DatabaseConnection> connection_;
    mutable std::mutex module_mutex_;

public:
    MongoDBModule();
    ~MongoDBModule() override;

    // Non-copyable
    MongoDBModule(const MongoDBModule&) = delete;
    MongoDBModule& operator=(const MongoDBModule&) = delete;

    void registerFunctions() override;

    // Basic connection method
    Symbols::ValuePtr connect(FunctionArguments& args);

    // Basic operations only - findOne and insertOne
    Symbols::ValuePtr findOne(FunctionArguments& args);
    Symbols::ValuePtr insertOne(FunctionArguments& args);

private:
    // Internal helper methods
    void initializeModule();
    void cleanupConnections();
    bool isConnected() const;

    // BSON conversion helpers
    bsoncxx::document::value convertToBSONDocument(const std::map<std::string, Symbols::ValuePtr>& document);
    Symbols::ValuePtr convertFromBSONDocument(const bsoncxx::document::view& view);
};

} // namespace Modules

#endif // MONGODBMODULE_MONGODBMODULE_HPP