// MariaDBModule implementation
#include "MariaDBModule.hpp"

#include <mysql.h>
#include <mysqld_error.h>
#include <errmsg.h>

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <sstream>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

// Static member definitions
std::unordered_map<std::string, std::unique_ptr<MariaDBClient>> MariaDBWrapper::connection_map_;

// File-scope static flag to prevent multiple registrations
static bool methodsRegistered = false;

// MariaDBClient implementation
MariaDBClient::MariaDBClient() : connection(nullptr), connected(false) {
    connection = mysql_init(nullptr);
    if (!connection) {
        throw DatabaseException("Failed to initialize MySQL connection");
    }
}

MariaDBClient::~MariaDBClient() {
    disconnect();
}

bool MariaDBClient::connect(const std::string& host, const std::string& username,
                           const std::string& password, const std::string& database, bool useSSL) {
    std::lock_guard<std::mutex> lock(client_mutex);

    if (connected) {
        disconnect();
    }

    if (!connection) {
        connection = mysql_init(nullptr);
        if (!connection) {
            throw DatabaseException("Failed to initialize MySQL connection");
        }
    }

    // Set connection options
    mysql_options(connection, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    // Enable SSL if requested
    if (useSSL) {
        mysql_ssl_set(connection, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    // Attempt connection
    MYSQL* result = mysql_real_connect(connection, host.c_str(), username.c_str(),
                                     password.c_str(), database.c_str(), 0, nullptr, 0);

    if (!result) {
        std::string error = mysql_error(connection);
        cleanup();
        throw DatabaseException("Connection failed: " + error);
    }

    connected = true;
    return true;
}

void MariaDBClient::disconnect() {
    std::lock_guard<std::mutex> lock(client_mutex);
    if (connection && connected) {
        mysql_close(connection);
        connection = nullptr;
        connected = false;
    }
}

bool MariaDBClient::isConnected() const {
    std::lock_guard<std::mutex> lock(client_mutex);
    return connected && connection != nullptr;
}

Symbols::ValuePtr MariaDBClient::query(const std::string& sql) {
    std::lock_guard<std::mutex> lock(client_mutex);

    if (!checkConnection()) {
        throw DatabaseException("Not connected to database");
    }

    // Execute query
    if (mysql_query(connection, sql.c_str()) != 0) {
        std::string error = mysql_error(connection);
        throw DatabaseException("Query failed: " + error);
    }

    // Check if it's a SELECT query
    MYSQL_RES* result = mysql_store_result(connection);
    if (result) {
        // SELECT query - return result set
        return executeSelect(result);
    } else {
        // Non-SELECT query - return affected rows
        if (mysql_errno(connection) != 0) {
            std::string error = mysql_error(connection);
            throw DatabaseException("Result retrieval failed: " + error);
        }
        return Symbols::ValuePtr(static_cast<int>(mysql_affected_rows(connection)));
    }
}

unsigned long long MariaDBClient::getLastInsertId() const {
    std::lock_guard<std::mutex> lock(client_mutex);
    if (!checkConnection()) {
        throw DatabaseException("Not connected to database");
    }
    return mysql_insert_id(connection);
}

unsigned long long MariaDBClient::getAffectedRows() const {
    std::lock_guard<std::mutex> lock(client_mutex);
    if (!checkConnection()) {
        throw DatabaseException("Not connected to database");
    }
    return mysql_affected_rows(connection);
}

void MariaDBClient::cleanup() {
    if (connection) {
        mysql_close(connection);
        connection = nullptr;
    }
    connected = false;
}

bool MariaDBClient::checkConnection() const {
    if (!connected || !connection) {
        return false;
    }

    // Ping to check if connection is still alive
    if (mysql_ping(connection) != 0) {
        // Note: We can't modify connected here since this is const
        // The caller should handle disconnection
        return false;
    }

    return true;
}

Symbols::ValuePtr MariaDBClient::executeSelect(MYSQL_RES* result) {
    if (!result) {
        Symbols::ObjectMap empty_result;
        return Symbols::ValuePtr(empty_result, true);
    }

    Symbols::ObjectMap result_map;
    MYSQL_ROW row;
    MYSQL_FIELD* fields = mysql_fetch_fields(result);
    unsigned int num_fields = mysql_num_fields(result);

    // Process each row
    int row_index = 0;
    while ((row = mysql_fetch_row(result))) {
        Symbols::ObjectMap row_map;

        for (unsigned int i = 0; i < num_fields; ++i) {
            std::string field_name = fields[i].name;
            std::string field_value = row[i] ? row[i] : "";

            row_map[field_name] = Symbols::ValuePtr(field_value);
        }

        result_map[std::to_string(row_index)] = Symbols::ValuePtr(row_map, true);
        row_index++;
    }

    mysql_free_result(result);
    return Symbols::ValuePtr(result_map, true);
}

// MariaDBWrapper implementation
Symbols::ValuePtr MariaDBWrapper::construct(const std::vector<Symbols::ValuePtr>& args) {
    if (args.size() != 1) {
        throw DatabaseException("MariaDB::construct expects no parameters");
    }

    if (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT) {
        throw DatabaseException("MariaDB::construct must be called on MariaDB instance");
    }

    std::string objectId = args[0].toString();
    connection_map_[objectId] = std::make_unique<MariaDBClient>();

    return args[0];
}

Symbols::ValuePtr MariaDBWrapper::connect(const std::vector<Symbols::ValuePtr>& args) {
    if (args.size() != 5 && args.size() != 6) {
        throw DatabaseException("MariaDB::connect expects 5 or 6 arguments: objectId, host, username, password, database, [useSSL]");
    }

    if (args[1] != Symbols::Variables::Type::STRING ||
        args[2] != Symbols::Variables::Type::STRING ||
        args[3] != Symbols::Variables::Type::STRING ||
        args[4] != Symbols::Variables::Type::STRING ||
        (args.size() == 6 && args[5] != Symbols::Variables::Type::BOOLEAN)) {
        throw DatabaseException("Parameters must be strings for host/username/password/database and boolean for useSSL");
    }

    std::string objectId = args[0].toString();
    MariaDBClient* client = getClient(objectId);

    std::string host = args[1].get<std::string>();
    std::string username = args[2].get<std::string>();
    std::string password = args[3].get<std::string>();
    std::string database = args[4].get<std::string>();
    bool useSSL = (args.size() == 6) ? args[5].get<bool>() : false;

    bool success = client->connect(host, username, password, database, useSSL);
    return Symbols::ValuePtr(success);
}

Symbols::ValuePtr MariaDBWrapper::disconnect(const std::vector<Symbols::ValuePtr>& args) {
    std::string objectId = args[0].toString();
    MariaDBClient* client = getClient(objectId);
    client->disconnect();
    return Symbols::ValuePtr(true);
}

Symbols::ValuePtr MariaDBWrapper::isConnected(const std::vector<Symbols::ValuePtr>& args) {
    std::string objectId = args[0].toString();
    MariaDBClient* client = getClient(objectId);
    return Symbols::ValuePtr(client->isConnected());
}

Symbols::ValuePtr MariaDBWrapper::query(const std::vector<Symbols::ValuePtr>& args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw DatabaseException("MariaDB::query expects one string parameter: sql");
    }

    std::string objectId = args[0].toString();
    MariaDBClient* client = getClient(objectId);
    std::string sql = args[1].get<std::string>();

    return client->query(sql);
}

Symbols::ValuePtr MariaDBWrapper::getLastInsertId(const std::vector<Symbols::ValuePtr>& args) {
    std::string objectId = args[0].toString();
    MariaDBClient* client = getClient(objectId);
    return Symbols::ValuePtr(static_cast<int>(client->getLastInsertId()));
}

Symbols::ValuePtr MariaDBWrapper::getAffectedRows(const std::vector<Symbols::ValuePtr>& args) {
    std::string objectId = args[0].toString();
    MariaDBClient* client = getClient(objectId);
    return Symbols::ValuePtr(static_cast<int>(client->getAffectedRows()));
}

MariaDBClient* MariaDBWrapper::getClient(const std::string& objectId) {
    auto it = connection_map_.find(objectId);
    if (it == connection_map_.end()) {
        throw DatabaseException("MariaDB: client not properly initialized");
    }
    return it->second.get();
}

// MariaDBModule implementation
void MariaDBModule::registerFunctions() {
    // Register OOP classes
    registerOOPClasses();
}

void MariaDBModule::registerOOPClasses() {
    // Prevent multiple registrations
    if (methodsRegistered) {
        return;
    }
    methodsRegistered = true;

    // Register MariaDBConnection class
    REGISTER_CLASS("MariaDBConnection");

    // Register MariaDBConnection methods - check if they already exist to prevent duplicates
    std::vector<Symbols::FunctionParameterInfo> no_params = {};
    if (!Symbols::SymbolContainer::instance()->hasMethod("MariaDBConnection", "construct")) {
        REGISTER_METHOD("MariaDBConnection", "construct", no_params, MariaDBWrapper::construct,
                       Symbols::Variables::Type::CLASS, "Create new MariaDBConnection");
    }

    std::vector<Symbols::FunctionParameterInfo> connect_params = {
        {"host", Symbols::Variables::Type::STRING, "Database host"},
        {"username", Symbols::Variables::Type::STRING, "Database username"},
        {"password", Symbols::Variables::Type::STRING, "Database password"},
        {"database", Symbols::Variables::Type::STRING, "Database name"},
        {"useSSL", Symbols::Variables::Type::BOOLEAN, "Enable SSL connection (optional, default: false)"}
    };
    if (!Symbols::SymbolContainer::instance()->hasMethod("MariaDBConnection", "connect")) {
        REGISTER_METHOD("MariaDBConnection", "connect", connect_params, MariaDBWrapper::connect,
                       Symbols::Variables::Type::BOOLEAN, "Connect to database");
    }

    if (!Symbols::SymbolContainer::instance()->hasMethod("MariaDBConnection", "disconnect")) {
        REGISTER_METHOD("MariaDBConnection", "disconnect", no_params, MariaDBWrapper::disconnect,
                       Symbols::Variables::Type::BOOLEAN, "Disconnect from database");
    }

    if (!Symbols::SymbolContainer::instance()->hasMethod("MariaDBConnection", "isConnected")) {
        REGISTER_METHOD("MariaDBConnection", "isConnected", no_params, MariaDBWrapper::isConnected,
                       Symbols::Variables::Type::BOOLEAN, "Check if connected");
    }

    std::vector<Symbols::FunctionParameterInfo> query_param = {
        {"sql", Symbols::Variables::Type::STRING, "SQL query"}
    };
    if (!Symbols::SymbolContainer::instance()->hasMethod("MariaDBConnection", "query")) {
        REGISTER_METHOD("MariaDBConnection", "query", query_param, MariaDBWrapper::query,
                       Symbols::Variables::Type::OBJECT, "Execute SQL query");
    }

    if (!Symbols::SymbolContainer::instance()->hasMethod("MariaDBConnection", "getLastInsertId")) {
        REGISTER_METHOD("MariaDBConnection", "getLastInsertId", no_params, MariaDBWrapper::getLastInsertId,
                       Symbols::Variables::Type::INTEGER, "Get last insert ID");
    }

    if (!Symbols::SymbolContainer::instance()->hasMethod("MariaDBConnection", "getAffectedRows")) {
        REGISTER_METHOD("MariaDBConnection", "getAffectedRows", no_params, MariaDBWrapper::getAffectedRows,
                       Symbols::Variables::Type::INTEGER, "Get affected rows count");
    }
}

} // namespace Modules