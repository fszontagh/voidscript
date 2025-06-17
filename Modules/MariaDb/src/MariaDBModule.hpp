// MariaDBModule.hpp
#ifndef MARIADBMODULE_MARIADBMODULE_HPP
#define MARIADBMODULE_MARIADBMODULE_HPP

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"
#include <mariadb/mysql.h>

#include <memory>
#include <string>
#include <mutex>
#include <chrono>
#include <map>
#include <atomic>
#include <regex>
#include <vector>
#include <set>

namespace Modules {

// Forward declarations
class DatabaseConnection;
class ConnectionManager;

/**
 * @brief Custom exception hierarchy for database operations
 */
class DatabaseException : public Modules::Exception {
private:
    int error_code_;
    std::string sql_state_;

public:
    DatabaseException(const std::string& message, int error_code = 0, const std::string& sql_state = "")
        : Exception(message), error_code_(error_code), sql_state_(sql_state) {}
    
    int getErrorCode() const { return error_code_; }
    const std::string& getSQLState() const { return sql_state_; }
};

/**
 * @brief Connection-specific exceptions
 */
class ConnectionException : public DatabaseException {
public:
    ConnectionException(const std::string& message, int error_code = 0)
        : DatabaseException(message, error_code) {}
};

/**
 * @brief Query-specific exceptions
 */
class QueryException : public DatabaseException {
public:
    QueryException(const std::string& message, int error_code = 0, const std::string& sql_state = "")
        : DatabaseException(message, error_code, sql_state) {}
};

/**
 * @brief Security-specific exceptions
 */
class SecurityException : public DatabaseException {
public:
    SecurityException(const std::string& message)
        : DatabaseException(message) {}
};

/**
 * @brief Transaction-specific exceptions
 */
class TransactionException : public DatabaseException {
public:
    TransactionException(const std::string& message, int error_code = 0)
        : DatabaseException(message, error_code) {}
};

/**
 * @brief Security validator for input validation and SQL injection prevention
 *
 * This class provides comprehensive input validation, SQL injection detection,
 * and safe parameter validation for all database operations.
 */
class SecurityValidator {
public:
    // Input validation methods
    static bool validateTableName(const std::string& table_name);
    static bool validateColumnName(const std::string& column_name);
    static bool validateIdentifier(const std::string& identifier);
    
    // SQL injection detection and prevention
    static bool containsSQLInjection(const std::string& input);
    static std::string sanitizeInput(const std::string& input);
    static bool validateInput(const std::string& input, const std::string& type);
    
    // Parameter validation
    static void validateParameters(const std::vector<Symbols::ValuePtr>& parameters);
    static bool isValidParameterType(const Symbols::ValuePtr& param);
    
    // Query validation
    static void validateQuery(const std::string& query);
    static bool isSelectQuery(const std::string& query);
    static bool isDataModificationQuery(const std::string& query);
    
private:
    // Validation patterns and rules
    static const std::regex VALID_IDENTIFIER_PATTERN;
    static const std::regex VALID_TABLE_NAME_PATTERN;
    static const std::regex VALID_COLUMN_NAME_PATTERN;
    static const std::vector<std::string> SQL_INJECTION_PATTERNS;
    static const std::vector<std::string> DANGEROUS_KEYWORDS;
    static const std::set<std::string> RESERVED_WORDS;
    
    // Helper methods
    static std::string toLowerCase(const std::string& str);
    static bool containsDangerousPattern(const std::string& input);
    static void logSecurityViolation(const std::string& violation, const std::string& input);
};

/**
 * @brief Prepared statement wrapper for safe query execution
 *
 * This class wraps MySQL prepared statements and provides automatic parameter
 * binding, type checking, and memory management.
 */
class PreparedStatement {
private:
    MYSQL_STMT* stmt_;
    DatabaseConnection* connection_;
    std::vector<MYSQL_BIND> param_binds_;
    std::vector<MYSQL_BIND> result_binds_;
    std::string query_;
    bool is_prepared_;
    int parameter_count_;
    
    // Parameter storage for binding
    std::vector<std::string> string_params_;
    std::vector<int> int_params_;
    std::vector<double> double_params_;
    std::vector<char> bool_params_;
    std::vector<unsigned long> param_lengths_;

public:
    PreparedStatement(DatabaseConnection* connection, const std::string& query);
    ~PreparedStatement();
    
    // Non-copyable but movable
    PreparedStatement(const PreparedStatement&) = delete;
    PreparedStatement& operator=(const PreparedStatement&) = delete;
    PreparedStatement(PreparedStatement&&) noexcept;
    PreparedStatement& operator=(PreparedStatement&&) noexcept;
    
    // Parameter binding methods
    bool bindParameter(int index, const Symbols::ValuePtr& value);
    bool bindParameters(const std::vector<Symbols::ValuePtr>& values);
    void clearParameters();
    
    // Execution methods
    bool execute();
    Symbols::ValuePtr executeQuery();
    int executeUpdate();
    
    // Result handling
    bool fetch();
    Symbols::ValuePtr getResult();
    int getAffectedRows();
    uint64_t getLastInsertId();
    
    // Status methods
    bool isPrepared() const { return is_prepared_; }
    int getParameterCount() const { return parameter_count_; }
    const std::string& getQuery() const { return query_; }

private:
    bool prepare();
    void setupParameterBinds();
    void setupResultBinds();
    void cleanup();
    bool bindParameterByType(int index, const Symbols::ValuePtr& value);
    void validateParameterIndex(int index);
};

/**
 * @brief Query builder for safe SQL construction
 *
 * This class provides a fluent interface for building SQL queries safely
 * without string concatenation vulnerabilities.
 */
class QueryBuilder {
private:
    std::string base_query_;
    std::string table_name_;
    std::vector<std::string> select_columns_;
    std::vector<std::string> where_conditions_;
    std::vector<std::string> order_by_columns_;
    std::vector<Symbols::ValuePtr> parameters_;
    std::map<std::string, Symbols::ValuePtr> named_parameters_;
    int limit_count_;
    int offset_count_;
    bool has_limit_;
    bool has_offset_;

public:
    QueryBuilder();
    ~QueryBuilder() = default;
    
    // Query building methods
    QueryBuilder& select(const std::vector<std::string>& columns);
    QueryBuilder& select(const std::string& column);
    QueryBuilder& from(const std::string& table);
    QueryBuilder& where(const std::string& condition);
    QueryBuilder& whereEquals(const std::string& column, const Symbols::ValuePtr& value);
    QueryBuilder& orderBy(const std::string& column, bool ascending = true);
    QueryBuilder& limit(int count, int offset = 0);
    
    // Parameter binding
    QueryBuilder& bindParameter(const std::string& name, const Symbols::ValuePtr& value);
    QueryBuilder& bindParameters(const std::vector<Symbols::ValuePtr>& values);
    
    // Query generation
    std::string buildQuery();
    std::vector<Symbols::ValuePtr> getParameters() const;
    std::string buildInsertQuery(const std::map<std::string, Symbols::ValuePtr>& data);
    std::string buildUpdateQuery(const std::map<std::string, Symbols::ValuePtr>& data,
                                const std::map<std::string, Symbols::ValuePtr>& conditions);
    std::string buildDeleteQuery(const std::map<std::string, Symbols::ValuePtr>& conditions);
    
    // Validation and reset
    void validate();
    void reset();
    
private:
    void validateTableName(const std::string& table);
    void validateColumnNames(const std::vector<std::string>& columns);
    void validateColumnName(const std::string& column);
    std::string buildWhereClause();
    std::string buildOrderByClause();
    std::string buildLimitClause();
};

/**
 * @brief Connection configuration structure
 */
struct ConnectionConfig {
    std::string host = "localhost";
    int port = 3306;
    std::string database;
    std::string username;
    std::string password;
    std::string charset = "utf8mb4";
    bool use_ssl = false;
    std::chrono::seconds connection_timeout = std::chrono::seconds(30);
    bool auto_reconnect = true;
    
    // Validation method
    bool isValid() const {
        return !host.empty() && !username.empty() && !database.empty() && port > 0;
    }
};

/**
 * @brief RAII-compliant database connection wrapper
 * 
 * This class manages a single database connection with automatic resource cleanup,
 * connection health monitoring, and thread-safe operations.
 */
class DatabaseConnection {
private:
    MYSQL* mysql_handle_;
    std::string connection_id_;
    std::chrono::time_point<std::chrono::steady_clock> last_used_;
    std::atomic<bool> is_healthy_;
    mutable std::mutex connection_mutex_;
    ConnectionConfig config_;
    bool is_connected_;

public:
    // RAII constructor/destructor
    /**
     * @brief Construct a new Database Connection object
     *
     * Creates a new database connection with the specified configuration.
     * Validates configuration and initializes the MySQL handle with proper options.
     *
     * @param config Connection configuration containing host, port, database, credentials
     *
     * @throws ConnectionException If configuration is invalid or MySQL handle initialization fails
     */
    explicit DatabaseConnection(const ConnectionConfig& config);
    
    /**
     * @brief Destroy the Database Connection object
     *
     * Automatically closes the connection and cleans up resources.
     * Ensures proper RAII cleanup of MySQL resources.
     */
    ~DatabaseConnection();
    
    // Non-copyable but movable
    /**
     * @brief Deleted copy constructor
     *
     * DatabaseConnection objects cannot be copied to ensure proper resource management.
     */
    DatabaseConnection(const DatabaseConnection&) = delete;
    
    /**
     * @brief Deleted copy assignment operator
     *
     * DatabaseConnection objects cannot be copy-assigned to ensure proper resource management.
     */
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;
    
    /**
     * @brief Move constructor
     *
     * Transfers ownership of database connection resources from another DatabaseConnection.
     * The source object is left in a valid but empty state.
     *
     * @param other DatabaseConnection object to move from
     */
    DatabaseConnection(DatabaseConnection&&) noexcept;
    
    /**
     * @brief Move assignment operator
     *
     * Transfers ownership of database connection resources from another DatabaseConnection.
     * Cleans up current resources before taking ownership of new ones.
     *
     * @param other DatabaseConnection object to move from
     * @return DatabaseConnection& Reference to this object
     */
    DatabaseConnection& operator=(DatabaseConnection&&) noexcept;
    
    // Connection management
    /**
     * @brief Establish connection to the database
     *
     * Attempts to connect to the MySQL database using the configured parameters.
     * Sets up SSL, charset, timeouts, and other connection options.
     *
     * @return bool True if connection successful, false otherwise
     *
     * @throws ConnectionException If connection parameters are invalid or connection fails
     */
    bool connect();
    
    /**
     * @brief Disconnect from the database
     *
     * Cleanly closes the database connection and marks it as disconnected.
     * Safe to call multiple times or on already disconnected connections.
     */
    void disconnect();
    
    /**
     * @brief Reconnect to the database
     *
     * Closes existing connection and establishes a new one using the same configuration.
     * Useful for recovering from connection timeouts or network issues.
     *
     * @return bool True if reconnection successful, false otherwise
     *
     * @throws ConnectionException If reconnection fails or MySQL handle cannot be reinitialized
     */
    bool reconnect();
    
    /**
     * @brief Check if currently connected to database
     *
     * Returns the current connection status based on internal state tracking.
     *
     * @return bool True if connected, false otherwise
     */
    bool isConnected() const;
    
    /**
     * @brief Check if connection is healthy and operational
     *
     * Performs comprehensive health checks including ping test and query validation.
     * More thorough than isConnected() as it actively tests the connection.
     *
     * @return bool True if connection is healthy, false otherwise
     */
    bool isHealthy() const;
    
    // Basic query execution (Phase 1 - foundation only)
    /**
     * @brief Execute a SELECT query and return result set
     *
     * Executes the provided SQL query and returns a MySQL result set.
     * Caller is responsible for freeing the result set with mysql_free_result().
     *
     * @param query SQL SELECT query to execute
     * @return MYSQL_RES* Pointer to result set, or nullptr for non-SELECT queries
     *
     * @throws QueryException If query is empty, connection not available, or execution fails
     */
    MYSQL_RES* executeQuery(const std::string& query);
    
    /**
     * @brief Execute a non-SELECT query (INSERT, UPDATE, DELETE, etc.)
     *
     * Executes SQL statements that don't return result sets.
     * Updates internal state for affected rows and last insert ID.
     *
     * @param query SQL statement to execute
     * @return bool True if execution successful, false otherwise
     *
     * @throws QueryException If query is empty, connection not available, or execution fails
     */
    bool executeNonQuery(const std::string& query);
    
    // Utilities
    /**
     * @brief Escape string for safe SQL usage
     *
     * Uses MySQL's built-in escaping to prevent SQL injection.
     * Properly handles special characters, quotes, and null bytes.
     *
     * @param input String to escape
     * @return std::string Escaped string safe for SQL queries
     *
     * @throws ConnectionException If MySQL handle not available for escaping
     */
    std::string escapeString(const std::string& input);
    
    /**
     * @brief Get the auto-increment ID from last INSERT
     *
     * Returns the auto-generated ID from the most recent INSERT statement
     * that inserted a row into a table with an AUTO_INCREMENT column.
     *
     * @return uint64_t Last insert ID, or 0 if no auto-increment ID available
     *
     * @throws ConnectionException If MySQL handle not available
     */
    uint64_t getLastInsertId();
    
    /**
     * @brief Get number of rows affected by last statement
     *
     * Returns the number of rows changed, deleted, or inserted by the
     * most recent UPDATE, DELETE, or INSERT statement.
     *
     * @return uint64_t Number of affected rows
     *
     * @throws ConnectionException If MySQL handle not available
     */
    uint64_t getAffectedRows();
    
    /**
     * @brief Get last MySQL error message
     *
     * Returns the error message from the most recent MySQL operation.
     * Useful for debugging and error reporting.
     *
     * @return std::string Error message, or "MySQL handle not available" if no handle
     */
    std::string getError();
    
    /**
     * @brief Get unique connection identifier
     *
     * Returns the unique ID assigned to this connection instance.
     * Useful for logging and connection tracking.
     *
     * @return const std::string& Connection ID string
     */
    const std::string& getConnectionId() const { return connection_id_; }
    
    // Health monitoring
    /**
     * @brief Update the last used timestamp
     *
     * Records the current time as the last activity time for this connection.
     * Used for connection timeout and health monitoring.
     */
    void updateLastUsed();
    
    /**
     * @brief Perform comprehensive connection health check
     *
     * Executes ping test, validation queries, and connection statistics checks.
     * Updates internal health status based on results.
     *
     * @return bool True if connection is healthy, false otherwise
     */
    bool checkHealth();
    
    // Get underlying handle for advanced operations (use with caution)
    /**
     * @brief Get raw MySQL handle for advanced operations
     *
     * Returns the underlying MYSQL* handle for direct MySQL API usage.
     * Use with extreme caution as this bypasses connection safety checks.
     *
     * @return MYSQL* Raw MySQL connection handle
     */
    MYSQL* getHandle() const { return mysql_handle_; }

private:
    bool initializeConnection();
    void cleanup();
    void generateConnectionId();
    bool performHealthCheck();
};

/**
 * @brief Result set handling for query execution
 *
 * This class provides streaming access to query results with efficient memory usage
 * and type-safe value retrieval capabilities.
 */
class ResultSet {
private:
    MYSQL_RES* result_;
    MYSQL_ROW current_row_;
    unsigned long* row_lengths_;
    std::vector<std::string> column_names_;
    std::vector<enum_field_types> column_types_;
    unsigned int num_fields_;
    my_ulonglong num_rows_;
    my_ulonglong current_row_index_;
    bool has_current_row_;
    bool owns_result_;

public:
    /**
     * @brief Construct a new Result Set object
     *
     * Creates a result set wrapper around a MySQL result set.
     * Initializes metadata and prepares for row iteration.
     *
     * @param result MySQL result set pointer from mysql_store_result()
     */
    explicit ResultSet(MYSQL_RES* result);
    
    /**
     * @brief Destroy the Result Set object
     *
     * Automatically frees the MySQL result set if owned by this object.
     * Ensures proper cleanup of MySQL resources.
     */
    ~ResultSet();
    
    // Non-copyable but movable
    /**
     * @brief Deleted copy constructor
     *
     * ResultSet objects cannot be copied to ensure proper resource management.
     */
    ResultSet(const ResultSet&) = delete;
    
    /**
     * @brief Deleted copy assignment operator
     *
     * ResultSet objects cannot be copy-assigned to ensure proper resource management.
     */
    ResultSet& operator=(const ResultSet&) = delete;
    
    /**
     * @brief Move constructor
     *
     * Transfers ownership of result set resources from another ResultSet.
     * The source object is left in a valid but empty state.
     *
     * @param other ResultSet object to move from
     */
    ResultSet(ResultSet&& other) noexcept;
    
    /**
     * @brief Move assignment operator
     *
     * Transfers ownership of result set resources from another ResultSet.
     * Cleans up current resources before taking ownership of new ones.
     *
     * @param other ResultSet object to move from
     * @return ResultSet& Reference to this object
     */
    ResultSet& operator=(ResultSet&& other) noexcept;
    
    // Row navigation
    /**
     * @brief Move to the next row in the result set
     *
     * Advances the internal cursor to the next row and updates current row data.
     * Must be called before accessing column data for the first time.
     *
     * @return bool True if next row available, false if no more rows
     */
    bool next();
    
    /**
     * @brief Check if more rows are available
     *
     * Determines if there are additional rows beyond the current position
     * without modifying the cursor position.
     *
     * @return bool True if more rows available, false otherwise
     */
    bool hasNext() const;
    
    /**
     * @brief Reset cursor to beginning of result set
     *
     * Moves the cursor back to before the first row, requiring next()
     * to be called to access the first row again.
     */
    void reset();
    
    /**
     * @brief Move to the first row in the result set
     *
     * Resets the cursor and advances to the first row if available.
     * Convenience method combining reset() and next().
     *
     * @return bool True if first row available, false if result set is empty
     */
    bool first();
    
    /**
     * @brief Move to the last row in the result set
     *
     * Jumps directly to the last row in the result set.
     *
     * @return bool True if last row available, false if result set is empty
     */
    bool last();
    
    // Column access by index
    /**
     * @brief Get column value as string by index
     *
     * Retrieves the value from the specified column in the current row as a string.
     * All MySQL data types are converted to string representation.
     *
     * @param columnIndex Zero-based column index
     * @return std::string Column value as string, empty string if NULL
     *
     * @throws QueryException If column index is out of range or no current row
     */
    std::string getString(int columnIndex) const;
    
    /**
     * @brief Get column value as integer by index
     *
     * Retrieves and converts the column value to an integer.
     * Performs safe conversion with error handling.
     *
     * @param columnIndex Zero-based column index
     * @return int Column value as integer, 0 if NULL or conversion fails
     *
     * @throws QueryException If column index is out of range, no current row, or conversion fails
     */
    int getInt(int columnIndex) const;
    
    /**
     * @brief Get column value as double by index
     *
     * Retrieves and converts the column value to a double-precision floating point.
     * Performs safe conversion with error handling.
     *
     * @param columnIndex Zero-based column index
     * @return double Column value as double, 0.0 if NULL or conversion fails
     *
     * @throws QueryException If column index is out of range, no current row, or conversion fails
     */
    double getDouble(int columnIndex) const;
    
    /**
     * @brief Get column value as boolean by index
     *
     * Retrieves and converts the column value to boolean.
     * Considers "0", "false", and NULL as false, everything else as true.
     *
     * @param columnIndex Zero-based column index
     * @return bool Column value as boolean
     *
     * @throws QueryException If column index is out of range or no current row
     */
    bool getBool(int columnIndex) const;
    
    /**
     * @brief Check if column value is NULL by index
     *
     * Determines if the column contains a SQL NULL value.
     *
     * @param columnIndex Zero-based column index
     * @return bool True if column is NULL, false otherwise
     *
     * @throws QueryException If column index is out of range or no current row
     */
    bool isNull(int columnIndex) const;
    
    // Column access by name
    /**
     * @brief Get column value as string by name
     *
     * Retrieves the value from the named column in the current row as a string.
     * Convenience method that looks up column index by name.
     *
     * @param columnName Column name to retrieve
     * @return std::string Column value as string, empty string if NULL
     *
     * @throws QueryException If column name not found or no current row
     */
    std::string getString(const std::string& columnName) const;
    
    /**
     * @brief Get column value as integer by name
     *
     * Retrieves and converts the named column value to an integer.
     *
     * @param columnName Column name to retrieve
     * @return int Column value as integer, 0 if NULL or conversion fails
     *
     * @throws QueryException If column name not found, no current row, or conversion fails
     */
    int getInt(const std::string& columnName) const;
    
    /**
     * @brief Get column value as double by name
     *
     * Retrieves and converts the named column value to a double.
     *
     * @param columnName Column name to retrieve
     * @return double Column value as double, 0.0 if NULL or conversion fails
     *
     * @throws QueryException If column name not found, no current row, or conversion fails
     */
    double getDouble(const std::string& columnName) const;
    
    /**
     * @brief Get column value as boolean by name
     *
     * Retrieves and converts the named column value to boolean.
     *
     * @param columnName Column name to retrieve
     * @return bool Column value as boolean
     *
     * @throws QueryException If column name not found or no current row
     */
    bool getBool(const std::string& columnName) const;
    
    /**
     * @brief Check if column value is NULL by name
     *
     * Determines if the named column contains a SQL NULL value.
     *
     * @param columnName Column name to check
     * @return bool True if column is NULL, false otherwise
     *
     * @throws QueryException If column name not found or no current row
     */
    bool isNull(const std::string& columnName) const;
    
    // Metadata
    /**
     * @brief Get the number of columns in the result set
     *
     * Returns the total number of columns/fields in the result set.
     *
     * @return unsigned int Number of columns
     */
    unsigned int getColumnCount() const { return num_fields_; }
    
    /**
     * @brief Get the total number of rows in the result set
     *
     * Returns the total number of rows available in the result set.
     *
     * @return my_ulonglong Total number of rows
     */
    my_ulonglong getRowCount() const { return num_rows_; }
    
    /**
     * @brief Get the current row index
     *
     * Returns the zero-based index of the current row position.
     *
     * @return my_ulonglong Current row index
     */
    my_ulonglong getCurrentRowIndex() const { return current_row_index_; }
    
    /**
     * @brief Get column name by index
     *
     * Returns the name of the column at the specified index.
     *
     * @param index Zero-based column index
     * @return std::string Column name
     *
     * @throws QueryException If column index is out of range
     */
    std::string getColumnName(int index) const;
    
    /**
     * @brief Get column data type by index
     *
     * Returns the MySQL field type for the column at the specified index.
     *
     * @param index Zero-based column index
     * @return enum_field_types MySQL field type enumeration
     *
     * @throws QueryException If column index is out of range
     */
    enum_field_types getColumnType(int index) const;
    
    /**
     * @brief Get all column names
     *
     * Returns a vector containing all column names in the result set.
     *
     * @return std::vector<std::string> Vector of column names
     */
    std::vector<std::string> getColumnNames() const { return column_names_; }
    
    // Utility
    /**
     * @brief Convert result set to VoidScript object
     *
     * Creates a VoidScript-compatible object representation of the result set,
     * including metadata and current row data.
     *
     * @return Symbols::ValuePtr VoidScript object containing result set data
     */
    Symbols::ValuePtr toVoidScriptObject() const;
    
private:
    void initializeMetadata();
    int getColumnIndex(const std::string& columnName) const;
    void validateColumnIndex(int index) const;
    std::string getRawValue(int columnIndex) const;
    void cleanup();
};

/**
 * @brief Batch processor for efficient bulk operations
 *
 * This class handles batch INSERT, UPDATE, and DELETE operations with
 * transaction support and error handling.
 */
class BatchProcessor {
private:
    DatabaseConnection* connection_;
    std::string operation_type_;
    std::string table_name_;
    std::vector<std::map<std::string, Symbols::ValuePtr>> batch_data_;
    size_t batch_size_limit_;
    bool use_transactions_;
    
public:
    explicit BatchProcessor(DatabaseConnection* connection, const std::string& operation_type);
    ~BatchProcessor() = default;
    
    // Configuration
    void setBatchSizeLimit(size_t limit) { batch_size_limit_ = limit; }
    void setUseTransactions(bool use_transactions) { use_transactions_ = use_transactions; }
    void setTableName(const std::string& table_name) { table_name_ = table_name; }
    
    // Batch operations
    void addBatchData(const std::map<std::string, Symbols::ValuePtr>& data);
    void addBatchData(const std::vector<std::map<std::string, Symbols::ValuePtr>>& data_list);
    void clearBatch();
    
    // Execution
    std::vector<int> executeBatch();
    int executeInsertBatch();
    int executeUpdateBatch(const std::map<std::string, Symbols::ValuePtr>& conditions);
    int executeDeleteBatch(const std::map<std::string, Symbols::ValuePtr>& conditions);
    
    // Status
    size_t getBatchSize() const { return batch_data_.size(); }
    bool isEmpty() const { return batch_data_.empty(); }
    
private:
    void validateBatchData();
    void validateTableName();
    std::string buildBatchInsertQuery();
    std::string buildBatchUpdateQuery(const std::map<std::string, Symbols::ValuePtr>& conditions);
    std::string buildBatchDeleteQuery(const std::map<std::string, Symbols::ValuePtr>& conditions);
    std::vector<Symbols::ValuePtr> flattenBatchParameters();
};

/**
 * @brief Query executor for comprehensive database operations
 *
 * This class provides the main interface for executing database operations
 * with full CRUD support, batch processing, and schema operations.
 */
class QueryExecutor {
private:
    ConnectionManager* connection_manager_;
    std::map<std::string, std::unique_ptr<PreparedStatement>> cached_statements_;
    mutable std::mutex executor_mutex_;
    
public:
    explicit QueryExecutor(ConnectionManager* connection_manager);
    ~QueryExecutor();
    
    // Non-copyable
    QueryExecutor(const QueryExecutor&) = delete;
    QueryExecutor& operator=(const QueryExecutor&) = delete;
    
    // Core query execution
    std::unique_ptr<ResultSet> executeQuery(const std::string& query,
                                          const std::vector<Symbols::ValuePtr>& parameters = {},
                                          DatabaseConnection* connection = nullptr);
    bool executeNonQuery(const std::string& query,
                        const std::vector<Symbols::ValuePtr>& parameters = {},
                        DatabaseConnection* connection = nullptr);
    
    // SELECT operations
    std::unique_ptr<ResultSet> select(const std::string& table,
                                    const std::vector<std::string>& columns = {"*"},
                                    const std::map<std::string, Symbols::ValuePtr>& conditions = {},
                                    const std::string& orderBy = "",
                                    int limit = -1, int offset = 0,
                                    DatabaseConnection* connection = nullptr);
    
    Symbols::ValuePtr selectOne(const std::string& table,
                               const std::vector<std::string>& columns = {"*"},
                               const std::map<std::string, Symbols::ValuePtr>& conditions = {},
                               DatabaseConnection* connection = nullptr);
    
    std::string selectColumn(const std::string& table, const std::string& column,
                           const std::map<std::string, Symbols::ValuePtr>& conditions = {},
                           int columnIndex = 0,
                           DatabaseConnection* connection = nullptr);
    
    Symbols::ValuePtr selectScalar(const std::string& table, const std::string& column,
                                  const std::map<std::string, Symbols::ValuePtr>& conditions = {},
                                  DatabaseConnection* connection = nullptr);
    
    // INSERT operations
    uint64_t insert(const std::string& table,
                   const std::map<std::string, Symbols::ValuePtr>& data,
                   DatabaseConnection* connection = nullptr);
    
    std::vector<uint64_t> insertBatch(const std::string& table,
                                     const std::vector<std::map<std::string, Symbols::ValuePtr>>& dataArray,
                                     DatabaseConnection* connection = nullptr);
    
    uint64_t insertAndGetId(const std::string& table,
                           const std::map<std::string, Symbols::ValuePtr>& data,
                           DatabaseConnection* connection = nullptr);
    
    // UPDATE operations
    int update(const std::string& table,
              const std::map<std::string, Symbols::ValuePtr>& data,
              const std::map<std::string, Symbols::ValuePtr>& conditions,
              DatabaseConnection* connection = nullptr);
    
    std::vector<int> updateBatch(const std::string& table,
                                const std::vector<std::map<std::string, Symbols::ValuePtr>>& dataArray,
                                const std::string& keyColumn,
                                DatabaseConnection* connection = nullptr);
    
    // DELETE operations
    int deleteRecord(const std::string& table,
                    const std::map<std::string, Symbols::ValuePtr>& conditions,
                    DatabaseConnection* connection = nullptr);
    
    std::vector<int> deleteBatch(const std::string& table,
                                const std::vector<Symbols::ValuePtr>& keyValues,
                                const std::string& keyColumn,
                                DatabaseConnection* connection = nullptr);
    
    // Schema operations
    bool createTable(const std::string& tableName,
                    const std::map<std::string, std::string>& columns,
                    const std::vector<std::string>& constraints = {},
                    DatabaseConnection* connection = nullptr);
    
    bool dropTable(const std::string& tableName,
                  bool ifExists = true,
                  DatabaseConnection* connection = nullptr);
    
    bool createIndex(const std::string& tableName,
                    const std::vector<std::string>& columns,
                    const std::string& indexName = "",
                    bool unique = false,
                    DatabaseConnection* connection = nullptr);
    
    bool dropIndex(const std::string& tableName,
                  const std::string& indexName,
                  bool ifExists = true,
                  DatabaseConnection* connection = nullptr);
    
    // Utility methods
    uint64_t getLastInsertId(DatabaseConnection* connection = nullptr);
    uint64_t getAffectedRows(DatabaseConnection* connection = nullptr);
    uint64_t getRowCount(const std::string& table,
                        const std::map<std::string, Symbols::ValuePtr>& conditions = {},
                        DatabaseConnection* connection = nullptr);
    
    // Prepared statement caching
    void clearStatementCache();
    size_t getCacheSize() const;
    
private:
    // Helper methods
    std::string buildSelectQuery(const std::string& table,
                               const std::vector<std::string>& columns,
                               const std::map<std::string, Symbols::ValuePtr>& conditions,
                               const std::string& orderBy,
                               int limit, int offset);
    
    std::string buildInsertQuery(const std::string& table,
                               const std::map<std::string, Symbols::ValuePtr>& data);
    
    std::string buildUpdateQuery(const std::string& table,
                               const std::map<std::string, Symbols::ValuePtr>& data,
                               const std::map<std::string, Symbols::ValuePtr>& conditions);
    
    std::string buildDeleteQuery(const std::string& table,
                               const std::map<std::string, Symbols::ValuePtr>& conditions);
    
    std::vector<Symbols::ValuePtr> extractParameters(const std::map<std::string, Symbols::ValuePtr>& data);
    std::vector<Symbols::ValuePtr> combineParameters(const std::vector<Symbols::ValuePtr>& params1,
                                                    const std::vector<Symbols::ValuePtr>& params2);
    
    DatabaseConnection* getConnection(DatabaseConnection* provided_connection);
    std::string generateStatementKey(const std::string& query);
    void validateTableName(const std::string& tableName);
    void validateColumnNames(const std::vector<std::string>& columns);
};

/**
 * @brief Savepoint management for nested transactions
 *
 * This class provides savepoint functionality for creating rollback points
 * within transactions, enabling complex transaction management scenarios.
 */
class Savepoint {
private:
    std::string name_;
    DatabaseConnection* connection_;
    bool is_active_;
    std::chrono::time_point<std::chrono::steady_clock> created_at_;

public:
    explicit Savepoint(const std::string& name, DatabaseConnection* connection);
    ~Savepoint();
    
    // Non-copyable but movable
    Savepoint(const Savepoint&) = delete;
    Savepoint& operator=(const Savepoint&) = delete;
    Savepoint(Savepoint&& other) noexcept;
    Savepoint& operator=(Savepoint&& other) noexcept;
    
    // Savepoint operations
    bool create();
    bool rollbackTo();
    bool release();
    
    // Status methods
    bool isActive() const { return is_active_; }
    const std::string& getName() const { return name_; }
    std::chrono::time_point<std::chrono::steady_clock> getCreatedAt() const { return created_at_; }
    
private:
    void cleanup();
};

/**
 * @brief RAII transaction scope for automatic transaction management
 *
 * This class provides exception-safe transaction handling with automatic
 * rollback on destruction if the transaction hasn't been committed.
 */
class TransactionScope {
private:
    DatabaseConnection* connection_;
    bool transaction_active_;
    bool auto_rollback_enabled_;
    bool committed_;
    std::stack<std::unique_ptr<Savepoint>> savepoint_stack_;
    mutable std::mutex scope_mutex_;

public:
    explicit TransactionScope(DatabaseConnection* connection, bool auto_rollback = true);
    ~TransactionScope();
    
    // Non-copyable and non-movable for safety
    TransactionScope(const TransactionScope&) = delete;
    TransactionScope& operator=(const TransactionScope&) = delete;
    TransactionScope(TransactionScope&&) = delete;
    TransactionScope& operator=(TransactionScope&&) = delete;
    
    // Transaction control
    bool begin();
    bool commit();
    bool rollback();
    
    // Savepoint management
    std::string createSavepoint(const std::string& name = "");
    bool rollbackToSavepoint(const std::string& name);
    bool releaseSavepoint(const std::string& name);
    
    // Status methods
    bool isActive() const { return transaction_active_; }
    bool isCommitted() const { return committed_; }
    size_t getSavepointCount() const { return savepoint_stack_.size(); }
    
    // Configuration
    void setAutoRollback(bool enabled) { auto_rollback_enabled_ = enabled; }
    bool getAutoRollback() const { return auto_rollback_enabled_; }

private:
    std::string generateSavepointName();
    void cleanup();
};

/**
 * @brief Advanced transaction manager with deadlock detection and isolation control
 *
 * This class provides comprehensive transaction management including nested
 * transactions, savepoints, isolation level control, and deadlock handling.
 */
class TransactionManager {
private:
    DatabaseConnection* connection_;
    std::stack<std::string> savepoint_stack_;
    std::atomic<bool> transaction_active_;
    mutable std::mutex transaction_mutex_;
    bool auto_rollback_enabled_;
    std::string current_isolation_level_;
    bool auto_commit_enabled_;
    
    // Deadlock detection
    std::chrono::milliseconds deadlock_timeout_;
    int max_retry_attempts_;
    std::chrono::milliseconds retry_backoff_base_;
    
    // Transaction statistics
    std::atomic<int> transaction_count_;
    std::atomic<int> rollback_count_;
    std::atomic<int> deadlock_count_;

public:
    enum class IsolationLevel {
        READ_UNCOMMITTED,
        READ_COMMITTED,
        REPEATABLE_READ,
        SERIALIZABLE
    };

    explicit TransactionManager(DatabaseConnection* connection);
    ~TransactionManager();
    
    // Non-copyable
    TransactionManager(const TransactionManager&) = delete;
    TransactionManager& operator=(const TransactionManager&) = delete;
    
    // Core transaction lifecycle
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    
    // Savepoint management
    std::string createSavepoint(const std::string& name = "");
    bool rollbackToSavepoint(const std::string& savepoint_name);
    bool releaseSavepoint(const std::string& savepoint_name);
    
    // Transaction status
    bool isInTransaction() const { return transaction_active_.load(); }
    size_t getSavepointCount() const;
    
    // Isolation level management
    bool setIsolationLevel(IsolationLevel level);
    IsolationLevel getIsolationLevel() const;
    std::string getIsolationLevelString() const { return current_isolation_level_; }
    
    // Auto-commit control
    bool setAutoCommit(bool enabled);
    bool getAutoCommit() const { return auto_commit_enabled_; }
    
    // Advanced transaction features
    template<typename Func>
    bool withTransaction(Func&& callback);
    
    template<typename Func>
    bool withSavepoint(const std::string& name, Func&& callback);
    
    // Deadlock detection and recovery
    bool detectDeadlock();
    bool executeWithRetry(std::function<bool()> operation, int max_retries = -1);
    
    // Configuration
    void setDeadlockTimeout(std::chrono::milliseconds timeout) { deadlock_timeout_ = timeout; }
    void setMaxRetryAttempts(int attempts) { max_retry_attempts_ = attempts; }
    void setRetryBackoffBase(std::chrono::milliseconds backoff) { retry_backoff_base_ = backoff; }
    void enableAutoRollback(bool enable = true) { auto_rollback_enabled_ = enable; }
    
    // Statistics
    int getTransactionCount() const { return transaction_count_.load(); }
    int getRollbackCount() const { return rollback_count_.load(); }
    int getDeadlockCount() const { return deadlock_count_.load(); }
    void resetStatistics();

private:
    std::string generateSavepointName();
    std::string isolationLevelToString(IsolationLevel level) const;
    IsolationLevel stringToIsolationLevel(const std::string& level) const;
    void updateStatistics(bool committed, bool deadlock_detected = false);
    std::chrono::milliseconds calculateBackoff(int attempt) const;
    bool handleDeadlock();
};

/**
 * @brief Enhanced MariaDB module with comprehensive query execution engine
 *
 * Phase 3 implementation adds:
 * - Complete CRUD operations with batch support
 * - Advanced result set handling with streaming
 * - Schema operations (create/drop tables, indexes)
 * - Performance-optimized query execution
 * - Memory-efficient result processing
 *
 * Phase 4 implementation adds:
 * - Advanced transaction management with ACID compliance
 * - Savepoint support for nested transactions
 * - RAII transaction scopes for exception safety
 * - Deadlock detection and automatic retry mechanisms
 * - Transaction isolation level control
 * - Comprehensive transaction statistics and monitoring
 */
class MariaDBModule : public BaseModule {
private:
    std::unique_ptr<ConnectionManager> connection_manager_;
    std::unique_ptr<QueryExecutor> query_executor_;
    std::unique_ptr<TransactionManager> transaction_manager_;
    mutable std::mutex module_mutex_;
    
    // Connection state tracking
    std::map<std::string, std::shared_ptr<DatabaseConnection>> active_connections_;
    
    // Phase 4: Transaction management state
    std::map<std::string, std::unique_ptr<TransactionScope>> active_transaction_scopes_;
    mutable std::mutex transaction_mutex_;
    
public:
    MariaDBModule();
    ~MariaDBModule() override;
    
    // Non-copyable
    MariaDBModule(const MariaDBModule&) = delete;
    MariaDBModule& operator=(const MariaDBModule&) = delete;
    
    void registerFunctions() override;
    
    // Phase 1: Foundation connection management methods
    Symbols::ValuePtr connect(FunctionArguments& args);
    Symbols::ValuePtr disconnect(FunctionArguments& args);
    Symbols::ValuePtr isConnected(FunctionArguments& args);
    Symbols::ValuePtr reconnect(FunctionArguments& args);
    
    // Basic query methods (Phase 1 - foundation only)
    Symbols::ValuePtr query(FunctionArguments& args);
    Symbols::ValuePtr close(FunctionArguments& args);
    
    // Utility methods
    Symbols::ValuePtr escapeString(FunctionArguments& args);
    Symbols::ValuePtr getLastInsertId(FunctionArguments& args);
    Symbols::ValuePtr getAffectedRows(FunctionArguments& args);
    Symbols::ValuePtr getConnectionInfo(FunctionArguments& args);
    
    // Phase 2: Security framework methods
    Symbols::ValuePtr validateInput(FunctionArguments& args);
    Symbols::ValuePtr prepareStatement(FunctionArguments& args);
    Symbols::ValuePtr bindParameter(FunctionArguments& args);
    Symbols::ValuePtr executeQuery(FunctionArguments& args);
    Symbols::ValuePtr executePrepared(FunctionArguments& args);
    
    // Query building methods
    Symbols::ValuePtr buildSelectQuery(FunctionArguments& args);
    Symbols::ValuePtr buildInsertQuery(FunctionArguments& args);
    Symbols::ValuePtr buildUpdateQuery(FunctionArguments& args);
    Symbols::ValuePtr buildDeleteQuery(FunctionArguments& args);
    
    // Phase 3: Query Execution Engine methods
    // SELECT operations
    Symbols::ValuePtr select(FunctionArguments& args);
    Symbols::ValuePtr selectOne(FunctionArguments& args);
    Symbols::ValuePtr selectColumn(FunctionArguments& args);
    Symbols::ValuePtr selectScalar(FunctionArguments& args);
    
    // INSERT operations
    Symbols::ValuePtr insert(FunctionArguments& args);
    Symbols::ValuePtr insertBatch(FunctionArguments& args);
    Symbols::ValuePtr insertAndGetId(FunctionArguments& args);
    
    // UPDATE operations
    Symbols::ValuePtr update(FunctionArguments& args);
    Symbols::ValuePtr updateBatch(FunctionArguments& args);
    
    // DELETE operations
    Symbols::ValuePtr deleteRecord(FunctionArguments& args);
    Symbols::ValuePtr deleteBatch(FunctionArguments& args);
    
    // Schema operations
    Symbols::ValuePtr createTable(FunctionArguments& args);
    Symbols::ValuePtr dropTable(FunctionArguments& args);
    Symbols::ValuePtr createIndex(FunctionArguments& args);
    Symbols::ValuePtr dropIndex(FunctionArguments& args);
    
    // Utility operations
    Symbols::ValuePtr getRowCount(FunctionArguments& args);
    
    // Phase 4: Transaction management methods
    // Basic transaction control
    Symbols::ValuePtr beginTransaction(FunctionArguments& args);
    Symbols::ValuePtr commitTransaction(FunctionArguments& args);
    Symbols::ValuePtr rollbackTransaction(FunctionArguments& args);
    Symbols::ValuePtr isInTransaction(FunctionArguments& args);
    
    // Savepoint management
    Symbols::ValuePtr createSavepoint(FunctionArguments& args);
    Symbols::ValuePtr rollbackToSavepoint(FunctionArguments& args);
    Symbols::ValuePtr releaseSavepoint(FunctionArguments& args);
    
    // Transaction scope management
    Symbols::ValuePtr withTransaction(FunctionArguments& args);
    Symbols::ValuePtr withSavepoint(FunctionArguments& args);
    
    // Isolation level management
    Symbols::ValuePtr setIsolationLevel(FunctionArguments& args);
    Symbols::ValuePtr getIsolationLevel(FunctionArguments& args);
    
    // Auto-commit control
    Symbols::ValuePtr setAutoCommit(FunctionArguments& args);
    Symbols::ValuePtr getAutoCommit(FunctionArguments& args);
    
    // Advanced transaction features
    Symbols::ValuePtr detectDeadlock(FunctionArguments& args);
    Symbols::ValuePtr getTransactionStatistics(FunctionArguments& args);

private:
    // Phase 2: Security framework storage
    std::map<std::string, std::unique_ptr<PreparedStatement>> prepared_statements_;
    std::map<std::string, std::unique_ptr<QueryBuilder>> query_builders_;
    mutable std::mutex security_mutex_;
    
    // Internal helper methods
    void initializeModule();
    void cleanupConnections();
    std::shared_ptr<DatabaseConnection> getConnectionFromArgs(const FunctionArguments& args);
    void validateConnectionParameters(const FunctionArguments& args);
    std::string generateConnectionKey(const ConnectionConfig& config);
    
    // Error handling helpers
    void handleDatabaseError(MYSQL* handle, const std::string& operation);
    void logError(const std::string& error, const std::string& context);
    
    // Phase 2: Security framework helpers
    void initializeSecurityFramework();
    void cleanupSecurityResources();
    std::string generateStatementKey(const std::string& query);
    void validateSecurityParameters(const FunctionArguments& args);
    Symbols::ValuePtr sanitizeErrorMessage(const std::string& error);
    void logSecurityEvent(const std::string& event, const std::string& context);
    
    // Parameter validation helpers
    bool isValidParameterType(const Symbols::ValuePtr& param);
    void validateParameterCount(const std::vector<Symbols::ValuePtr>& params, int expected);
    std::string convertValueToString(const Symbols::ValuePtr& value);
    
    // Phase 3: Query execution helpers
    void initializeQueryExecutor();
    std::map<std::string, Symbols::ValuePtr> extractObjectMapFromArgs(const Symbols::ValuePtr& arg);
    std::vector<std::string> extractStringArrayFromArgs(const Symbols::ValuePtr& arg);
    std::vector<std::map<std::string, Symbols::ValuePtr>> extractDataArrayFromArgs(const Symbols::ValuePtr& arg);
    std::vector<Symbols::ValuePtr> extractValueArrayFromArgs(const Symbols::ValuePtr& arg);
    
    // Phase 4: Transaction management helpers
    void initializeTransactionManager();
    void cleanupTransactionResources();
    TransactionManager* getTransactionManager(const FunctionArguments& args);
    std::string generateTransactionScopeKey();
    void validateTransactionState(const FunctionArguments& args);
    
    // Transaction helper methods
    bool executeInTransaction(std::function<bool()> operation, bool use_savepoint = false);
    void handleTransactionError(const std::exception& e, const std::string& operation);
    void logTransactionEvent(const std::string& event, const std::string& context);
};

} // namespace Modules

#endif // MARIADBMODULE_MARIADBMODULE_HPP
