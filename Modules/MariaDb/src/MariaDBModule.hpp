// MariaDBModule.hpp
#ifndef MARIADBMODULE_HPP
#define MARIADBMODULE_HPP

#include <mysql.h>

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

/**
 * @brief Exception class for database-related errors
 */
class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const std::string& msg) : std::runtime_error(msg) {}
};

/**
 * @brief MariaDB client class for database operations
 */
class MariaDBClient {
private:
    MYSQL* connection = nullptr;
    bool connected = false;
    mutable std::mutex client_mutex;

public:
    MariaDBClient();
    ~MariaDBClient();

    /**
     * @brief Connect to MariaDB database
     * @param host Hostname or IP address
     * @param username Username
     * @param password Password
     * @param database Database name
     * @param useSSL Enable SSL connection (default: false)
     * @return true if connection successful
     */
    bool connect(const std::string& host, const std::string& username,
                 const std::string& password, const std::string& database, bool useSSL = false);

    /**
     * @brief Disconnect from database
     */
    void disconnect();

    /**
     * @brief Check if connected to database
     * @return true if connected
     */
    bool isConnected() const;

    /**
     * @brief Execute SQL query
     * @param sql SQL query string
     * @return For SELECT: result set as vector of maps, for others: affected rows
     */
    Symbols::ValuePtr query(const std::string& sql);

    /**
     * @brief Get last insert ID
     * @return Last insert ID
     */
    unsigned long long getLastInsertId() const;

    /**
     * @brief Get number of affected rows from last query
     * @return Number of affected rows
     */
    unsigned long long getAffectedRows() const;

private:
    void cleanup();
    bool checkConnection() const;
    Symbols::ValuePtr executeSelect(MYSQL_RES* result);
};

/**
 * @brief Connection wrapper for VoidScript OOP interface
 */
class MariaDBWrapper {
private:
    static std::unordered_map<std::string, std::unique_ptr<MariaDBClient>> connection_map_;

public:
    static Symbols::ValuePtr construct(const std::vector<Symbols::ValuePtr>& args);
    static Symbols::ValuePtr connect(const std::vector<Symbols::ValuePtr>& args);
    static Symbols::ValuePtr disconnect(const std::vector<Symbols::ValuePtr>& args);
    static Symbols::ValuePtr isConnected(const std::vector<Symbols::ValuePtr>& args);
    static Symbols::ValuePtr query(const std::vector<Symbols::ValuePtr>& args);
    static Symbols::ValuePtr getLastInsertId(const std::vector<Symbols::ValuePtr>& args);
    static Symbols::ValuePtr getAffectedRows(const std::vector<Symbols::ValuePtr>& args);

private:
    static MariaDBClient* getClient(const std::string& objectId);
};

/**
 * @brief MariaDB Module providing database connectivity and operations
 */
class MariaDBModule final : public BaseModule {
public:
    MariaDBModule() {
        setModuleName("MariaDB");
        setDescription("Provides MariaDB/MySQL database connectivity with support for connections, queries, and result handling through both legacy functions and OOP interface.");
    }

    /**
      * @brief Register this module's symbols (OOP classes).
       */
    void registerFunctions() override;

private:
    void registerOOPClasses();
};

} // namespace Modules

#endif // MARIADBMODULE_HPP