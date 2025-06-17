// MariaDBModule.cpp
#include "MariaDBModule.hpp"

#include <mariadb/mysql.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>
#include <random>
#include <iomanip>
#include <regex>
#include <algorithm>
#include <cctype>
#include <set>
#include <cstring>
#include <thread>
#include <chrono>
#include <functional>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

// Connection state management - replaced global static map with proper RAII
static std::atomic<int> next_connection_counter{1};

//=============================================================================
// DatabaseConnection Implementation
//=============================================================================

DatabaseConnection::DatabaseConnection(const ConnectionConfig& config)
    : mysql_handle_(nullptr)
    , config_(config)
    , is_healthy_(false)
    , is_connected_(false)
    , last_used_(std::chrono::steady_clock::now())
{
    if (!config_.isValid()) {
        throw ConnectionException("Invalid connection configuration provided");
    }
    
    generateConnectionId();
    
    // Initialize MySQL handle
    mysql_handle_ = mysql_init(nullptr);
    if (!mysql_handle_) {
        throw ConnectionException("Failed to initialize MySQL handle");
    }
    
    // Set connection options
    unsigned int timeout = static_cast<unsigned int>(config_.connection_timeout.count());
    mysql_options(mysql_handle_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(mysql_handle_, MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(mysql_handle_, MYSQL_OPT_WRITE_TIMEOUT, &timeout);
    
    // Set charset
    mysql_options(mysql_handle_, MYSQL_SET_CHARSET_NAME, config_.charset.c_str());
    
    // Enable auto-reconnect if configured
    bool reconnect = config_.auto_reconnect;
    mysql_options(mysql_handle_, MYSQL_OPT_RECONNECT, &reconnect);
    
}

DatabaseConnection::~DatabaseConnection() {
    cleanup();
}

DatabaseConnection::DatabaseConnection(DatabaseConnection&& other) noexcept
    : mysql_handle_(other.mysql_handle_)
    , connection_id_(std::move(other.connection_id_))
    , last_used_(other.last_used_)
    , is_healthy_(other.is_healthy_.load())
    , config_(std::move(other.config_))
    , is_connected_(other.is_connected_)
{
    other.mysql_handle_ = nullptr;
    other.is_healthy_ = false;
    other.is_connected_ = false;
}

DatabaseConnection& DatabaseConnection::operator=(DatabaseConnection&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        mysql_handle_ = other.mysql_handle_;
        connection_id_ = std::move(other.connection_id_);
        last_used_ = other.last_used_;
        is_healthy_ = other.is_healthy_.load();
        config_ = std::move(other.config_);
        is_connected_ = other.is_connected_;
        
        other.mysql_handle_ = nullptr;
        other.is_healthy_ = false;
        other.is_connected_ = false;
    }
    return *this;
}

bool DatabaseConnection::connect() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    if (is_connected_) {
        return true;
    }
    
    if (!mysql_handle_) {
        throw ConnectionException("MySQL handle not initialized");
    }
    
    return initializeConnection();
}

void DatabaseConnection::disconnect() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    if (mysql_handle_ && is_connected_) {
        mysql_close(mysql_handle_);
    }
    
    is_connected_ = false;
    is_healthy_ = false;
}

bool DatabaseConnection::reconnect() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    
    if (mysql_handle_ && is_connected_) {
        mysql_close(mysql_handle_);
    }
    
    // Reinitialize handle
    mysql_handle_ = mysql_init(nullptr);
    if (!mysql_handle_) {
        is_connected_ = false;
        is_healthy_ = false;
        throw ConnectionException("Failed to reinitialize MySQL handle during reconnect");
    }
    
    is_connected_ = false;
    is_healthy_ = false;
    
    return initializeConnection();
}

bool DatabaseConnection::isConnected() const {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    return is_connected_ && mysql_handle_ != nullptr;
}

bool DatabaseConnection::isHealthy() const {
    return is_healthy_.load() && isConnected();
}

MYSQL_RES* DatabaseConnection::executeQuery(const std::string& query) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    if (!isConnected()) {
        throw QueryException("Connection not established");
    }
    
    if (query.empty()) {
        throw QueryException("Query string cannot be empty");
    }
    
    updateLastUsed();
    
    
    if (mysql_query(mysql_handle_, query.c_str()) != 0) {
        std::string error_msg = mysql_error(mysql_handle_);
        int error_code = mysql_errno(mysql_handle_);
        is_healthy_ = false;
        
        throw QueryException("Query execution failed: " + error_msg, error_code);
    }
    
    MYSQL_RES* result = mysql_store_result(mysql_handle_);
    if (!result && mysql_field_count(mysql_handle_) > 0) {
        std::string error_msg = mysql_error(mysql_handle_);
        int error_code = mysql_errno(mysql_handle_);
        is_healthy_ = false;
        
        throw QueryException("Failed to store query result: " + error_msg, error_code);
    }
    
    return result;
}

bool DatabaseConnection::executeNonQuery(const std::string& query) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    if (!isConnected()) {
        throw QueryException("Connection not established");
    }
    
    if (query.empty()) {
        throw QueryException("Query string cannot be empty");
    }
    
    updateLastUsed();
    
    
    if (mysql_query(mysql_handle_, query.c_str()) != 0) {
        std::string error_msg = mysql_error(mysql_handle_);
        int error_code = mysql_errno(mysql_handle_);
        is_healthy_ = false;
        
        throw QueryException("Non-query execution failed: " + error_msg, error_code);
    }
    
    return true;
}

std::string DatabaseConnection::escapeString(const std::string& input) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    if (!mysql_handle_) {
        throw ConnectionException("MySQL handle not available for string escaping");
    }
    
    // Allocate buffer for escaped string (2 * input.length() + 1 is the maximum needed)
    std::string escaped;
    escaped.resize(2 * input.length() + 1);
    
    unsigned long escaped_length = mysql_real_escape_string(
        mysql_handle_, 
        &escaped[0], 
        input.c_str(), 
        input.length()
    );
    
    escaped.resize(escaped_length);
    return escaped;
}

uint64_t DatabaseConnection::getLastInsertId() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    if (!mysql_handle_) {
        throw ConnectionException("MySQL handle not available");
    }
    
    return mysql_insert_id(mysql_handle_);
}

uint64_t DatabaseConnection::getAffectedRows() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    if (!mysql_handle_) {
        throw ConnectionException("MySQL handle not available");
    }
    
    return mysql_affected_rows(mysql_handle_);
}

std::string DatabaseConnection::getError() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    if (!mysql_handle_) {
        return "MySQL handle not available";
    }
    
    return std::string(mysql_error(mysql_handle_));
}

void DatabaseConnection::updateLastUsed() {
    last_used_ = std::chrono::steady_clock::now();
}

bool DatabaseConnection::checkHealth() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    return performHealthCheck();
}

bool DatabaseConnection::initializeConnection() {
    // This method assumes connection_mutex_ is already locked
    
    
    MYSQL* result = mysql_real_connect(
        mysql_handle_,
        config_.host.c_str(),
        config_.username.c_str(),
        config_.password.c_str(),
        config_.database.c_str(),
        config_.port,
        nullptr,  // socket
        0         // client flags
    );
    
    if (!result) {
        std::string error_msg = mysql_error(mysql_handle_);
        int error_code = mysql_errno(mysql_handle_);
        
        
        throw ConnectionException("Failed to connect to database: " + error_msg, error_code);
    }
    
    is_connected_ = true;
    is_healthy_ = true;
    updateLastUsed();
    
    
    // Perform initial health check
    if (!performHealthCheck()) {
        is_healthy_ = false;
    }
    
    return true;
}

void DatabaseConnection::cleanup() {
    if (mysql_handle_) {
        if (is_connected_) {
            mysql_close(mysql_handle_);
        }
        mysql_handle_ = nullptr;
    }
    
    is_connected_ = false;
    is_healthy_ = false;
}

void DatabaseConnection::generateConnectionId() {
    std::stringstream ss;
    ss << "conn_" << next_connection_counter.fetch_add(1) << "_";
    
    // Add timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    ss << std::put_time(std::localtime(&time_t), "%H%M%S");
    
    connection_id_ = ss.str();
}

bool DatabaseConnection::performHealthCheck() {
    // This method assumes connection_mutex_ is already locked
    
    if (!mysql_handle_ || !is_connected_) {
        is_healthy_ = false;
        return false;
    }
    
    // Multi-step health check process
    bool healthy = true;
    
    // Step 1: Basic ping test
    int ping_result = mysql_ping(mysql_handle_);
    if (ping_result != 0) {
        healthy = false;
    }
    
    // Step 2: Test with a simple query if ping succeeds
    if (healthy) {
        try {
            MYSQL_RES* result = mysql_list_tables(mysql_handle_, nullptr);
            if (result) {
                mysql_free_result(result);
            } else {
                // If mysql_list_tables fails, try a simpler query
                if (mysql_query(mysql_handle_, "SELECT 1") != 0) {
                    healthy = false;
                } else {
                    MYSQL_RES* simple_result = mysql_store_result(mysql_handle_);
                    if (simple_result) {
                        mysql_free_result(simple_result);
                    } else {
                        healthy = false;
                    }
                }
            }
        } catch (...) {
            healthy = false;
        }
    }
    
    // Step 3: Check connection statistics and status
    if (healthy) {
        // Verify we can access connection info
        const char* info = mysql_get_host_info(mysql_handle_);
        if (!info) {
            healthy = false;
        }
        
        // Check if connection has been idle too long (configurable threshold)
        auto now = std::chrono::steady_clock::now();
        auto idle_time = std::chrono::duration_cast<std::chrono::minutes>(now - last_used_);
        if (idle_time.count() > 30) { // 30 minute idle threshold
            // Perform additional validation for long-idle connections
            if (mysql_query(mysql_handle_, "SELECT CONNECTION_ID()") != 0) {
                healthy = false;
            } else {
                MYSQL_RES* conn_result = mysql_store_result(mysql_handle_);
                if (conn_result) {
                    mysql_free_result(conn_result);
                } else {
                    healthy = false;
                }
            }
        }
    }
    
    // Update health status
    is_healthy_ = healthy;
    
    // If unhealthy, attempt to get error information for debugging
    if (!healthy) {
        std::string error_msg = mysql_error(mysql_handle_);
        int error_code = mysql_errno(mysql_handle_);
        
        // Mark connection as unhealthy
        is_connected_ = false;
    }
    
    return healthy;
}

//=============================================================================
// SecurityValidator Implementation (Phase 2)
//=============================================================================

// Static member definitions
const std::regex SecurityValidator::VALID_IDENTIFIER_PATTERN(R"(^[a-zA-Z][a-zA-Z0-9_]*$)");
const std::regex SecurityValidator::VALID_TABLE_NAME_PATTERN(R"(^[a-zA-Z][a-zA-Z0-9_]*$)");
const std::regex SecurityValidator::VALID_COLUMN_NAME_PATTERN(R"(^[a-zA-Z][a-zA-Z0-9_]*$)");

const std::vector<std::string> SecurityValidator::SQL_INJECTION_PATTERNS = {
    R"((\bUNION\b.*\bSELECT\b))",
    R"((;\s*DROP\s+TABLE))",
    R"((;\s*DELETE\s+FROM))",
    R"((;\s*INSERT\s+INTO))",
    R"((;\s*UPDATE\s+.*\bSET\b))",
    R"((\bOR\b\s+\d+\s*=\s*\d+))",
    R"((\bAND\b\s+\d+\s*=\s*\d+))",
    R"((--\s*))",
    R"((/\*.*\*/))",
    R"((\bEXEC\b|\bEXECUTE\b))",
    R"((\bSP_\w+))",
    R"((\bXP_\w+))"
};

const std::vector<std::string> SecurityValidator::DANGEROUS_KEYWORDS = {
    "DROP", "DELETE", "INSERT", "UPDATE", "ALTER", "CREATE", "TRUNCATE",
    "EXEC", "EXECUTE", "SP_", "XP_", "UNION", "SCRIPT", "JAVASCRIPT",
    "VBSCRIPT", "ONLOAD", "ONERROR", "EVAL"
};

const std::set<std::string> SecurityValidator::RESERVED_WORDS = {
    "SELECT", "FROM", "WHERE", "INSERT", "UPDATE", "DELETE", "DROP", "CREATE",
    "ALTER", "TABLE", "INDEX", "VIEW", "DATABASE", "SCHEMA", "PROCEDURE",
    "FUNCTION", "TRIGGER", "GRANT", "REVOKE", "COMMIT", "ROLLBACK", "TRANSACTION"
};

bool SecurityValidator::validateTableName(const std::string& table_name) {
    if (table_name.empty() || table_name.length() > 64) {
        return false;
    }
    
    // Check against pattern
    if (!std::regex_match(table_name, VALID_TABLE_NAME_PATTERN)) {
        return false;
    }
    
    // Check against reserved words
    std::string upper_name = toLowerCase(table_name);
    std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(), ::toupper);
    
    if (RESERVED_WORDS.find(upper_name) != RESERVED_WORDS.end()) {
        logSecurityViolation("Reserved word used as table name", table_name);
        return false;
    }
    
    return true;
}

bool SecurityValidator::validateColumnName(const std::string& column_name) {
    if (column_name.empty() || column_name.length() > 64) {
        return false;
    }
    
    // Check against pattern
    if (!std::regex_match(column_name, VALID_COLUMN_NAME_PATTERN)) {
        return false;
    }
    
    // Check against reserved words
    std::string upper_name = toLowerCase(column_name);
    std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(), ::toupper);
    
    if (RESERVED_WORDS.find(upper_name) != RESERVED_WORDS.end()) {
        logSecurityViolation("Reserved word used as column name", column_name);
        return false;
    }
    
    return true;
}

bool SecurityValidator::validateIdentifier(const std::string& identifier) {
    if (identifier.empty() || identifier.length() > 64) {
        return false;
    }
    
    return std::regex_match(identifier, VALID_IDENTIFIER_PATTERN);
}

bool SecurityValidator::containsSQLInjection(const std::string& input) {
    std::string upper_input = input;
    std::transform(upper_input.begin(), upper_input.end(), upper_input.begin(), ::toupper);
    
    // Check for SQL injection patterns
    for (const auto& pattern : SQL_INJECTION_PATTERNS) {
        std::regex regex_pattern(pattern, std::regex_constants::icase);
        if (std::regex_search(upper_input, regex_pattern)) {
            logSecurityViolation("SQL injection pattern detected", input);
            return true;
        }
    }
    
    // Check for dangerous keywords in suspicious contexts
    if (containsDangerousPattern(upper_input)) {
        logSecurityViolation("Dangerous keyword pattern detected", input);
        return true;
    }
    
    return false;
}

std::string SecurityValidator::sanitizeInput(const std::string& input) {
    if (input.empty()) {
        return input;
    }
    
    // Check for SQL injection first
    if (containsSQLInjection(input)) {
        throw SecurityException("Input contains potential SQL injection patterns");
    }
    
    // Basic sanitization - remove dangerous characters
    std::string sanitized = input;
    
    // Remove null bytes
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\0'), sanitized.end());
    
    // Remove or escape dangerous sequences
    std::regex dangerous_chars(R"([;<>&|`$(){}[\]\\])");
    sanitized = std::regex_replace(sanitized, dangerous_chars, "");
    
    return sanitized;
}

bool SecurityValidator::validateInput(const std::string& input, const std::string& type) {
    if (input.empty()) {
        return true; // Empty input is generally safe
    }
    
    // Check for SQL injection patterns
    if (containsSQLInjection(input)) {
        return false;
    }
    
    // Type-specific validation
    if (type == "table_name") {
        return validateTableName(input);
    } else if (type == "column_name") {
        return validateColumnName(input);
    } else if (type == "identifier") {
        return validateIdentifier(input);
    } else if (type == "string") {
        // General string validation
        return input.length() <= 65535; // Max TEXT length
    } else if (type == "integer") {
        // Validate integer format
        std::regex int_pattern(R"(^-?\d+$)");
        return std::regex_match(input, int_pattern);
    } else if (type == "float" || type == "double") {
        // Validate floating point format
        std::regex float_pattern(R"(^-?\d+(\.\d+)?([eE][+-]?\d+)?$)");
        return std::regex_match(input, float_pattern);
    }
    
    return true; // Default to safe for unknown types
}

void SecurityValidator::validateParameters(const std::vector<Symbols::ValuePtr>& parameters) {
    if (parameters.size() > 1000) { // Max parameter limit
        throw SecurityException("Too many parameters provided (max: 1000)");
    }
    
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (!isValidParameterType(parameters[i])) {
            throw SecurityException("Invalid parameter type at index " + std::to_string(i));
        }
    }
}

bool SecurityValidator::isValidParameterType(const Symbols::ValuePtr& param) {
    if (!param) {
        return false;
    }
    
    Symbols::Variables::Type type = param.getType();
    
    switch (type) {
        case Symbols::Variables::Type::STRING:
        case Symbols::Variables::Type::INTEGER:
        case Symbols::Variables::Type::FLOAT:
        case Symbols::Variables::Type::DOUBLE:
        case Symbols::Variables::Type::BOOLEAN:
        case Symbols::Variables::Type::NULL_TYPE:
            return true;
        default:
            return false;
    }
}

void SecurityValidator::validateQuery(const std::string& query) {
    if (query.empty()) {
        throw SecurityException("Query cannot be empty");
    }
    
    if (query.length() > 65536) { // Max query length
        throw SecurityException("Query too long (max: 64KB)");
    }
    
    // Check for SQL injection
    if (containsSQLInjection(query)) {
        throw SecurityException("Query contains potential SQL injection patterns");
    }
    
    // Additional query validation can be added here
}

bool SecurityValidator::isSelectQuery(const std::string& query) {
    std::string trimmed = query;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    
    if (trimmed.length() < 6) {
        return false;
    }
    
    std::string prefix = trimmed.substr(0, 6);
    std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::toupper);
    
    return prefix == "SELECT";
}

bool SecurityValidator::isDataModificationQuery(const std::string& query) {
    std::string trimmed = query;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    
    if (trimmed.length() < 6) {
        return false;
    }
    
    std::string prefix = trimmed.substr(0, 6);
    std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::toupper);
    
    return prefix == "INSERT" || prefix == "UPDATE" || prefix == "DELETE";
}

std::string SecurityValidator::toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool SecurityValidator::containsDangerousPattern(const std::string& input) {
    for (const auto& keyword : DANGEROUS_KEYWORDS) {
        if (input.find(keyword) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void SecurityValidator::logSecurityViolation(const std::string& violation, const std::string& input) {
}

//=============================================================================
// ConnectionManager Implementation (Basic Phase 1 version)
//=============================================================================

class ConnectionManager {
private:
    std::mutex manager_mutex_;
    
public:
    ConnectionManager() = default;
    ~ConnectionManager() = default;
    
    std::shared_ptr<DatabaseConnection> createConnection(const ConnectionConfig& config) {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        
        auto connection = std::make_shared<DatabaseConnection>(config);
        if (!connection->connect()) {
            throw ConnectionException("Failed to establish database connection");
        }
        
        return connection;
    }
    
    void validateConnection(const std::shared_ptr<DatabaseConnection>& connection) {
        if (!connection) {
            throw ConnectionException("Invalid connection pointer");
        }
        
        if (!connection->isHealthy()) {
            throw ConnectionException("Connection is not healthy");
        }
    }
};

//=============================================================================
// PreparedStatement Implementation (Phase 2)
//=============================================================================

PreparedStatement::PreparedStatement(DatabaseConnection* connection, const std::string& query)
    : stmt_(nullptr)
    , connection_(connection)
    , query_(query)
    , is_prepared_(false)
    , parameter_count_(0)
{
    if (!connection_) {
        throw SecurityException("Invalid connection provided to PreparedStatement");
    }
    
    // Validate query for security
    SecurityValidator::validateQuery(query_);
    
}

PreparedStatement::~PreparedStatement() {
    cleanup();
}

PreparedStatement::PreparedStatement(PreparedStatement&& other) noexcept
    : stmt_(other.stmt_)
    , connection_(other.connection_)
    , param_binds_(std::move(other.param_binds_))
    , result_binds_(std::move(other.result_binds_))
    , query_(std::move(other.query_))
    , is_prepared_(other.is_prepared_)
    , parameter_count_(other.parameter_count_)
    , string_params_(std::move(other.string_params_))
    , int_params_(std::move(other.int_params_))
    , double_params_(std::move(other.double_params_))
    , bool_params_(std::move(other.bool_params_))
    , param_lengths_(std::move(other.param_lengths_))
{
    other.stmt_ = nullptr;
    other.connection_ = nullptr;
    other.is_prepared_ = false;
    other.parameter_count_ = 0;
}

PreparedStatement& PreparedStatement::operator=(PreparedStatement&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        stmt_ = other.stmt_;
        connection_ = other.connection_;
        param_binds_ = std::move(other.param_binds_);
        result_binds_ = std::move(other.result_binds_);
        query_ = std::move(other.query_);
        is_prepared_ = other.is_prepared_;
        parameter_count_ = other.parameter_count_;
        string_params_ = std::move(other.string_params_);
        int_params_ = std::move(other.int_params_);
        double_params_ = std::move(other.double_params_);
        bool_params_ = std::move(other.bool_params_);
        param_lengths_ = std::move(other.param_lengths_);
        
        other.stmt_ = nullptr;
        other.connection_ = nullptr;
        other.is_prepared_ = false;
        other.parameter_count_ = 0;
    }
    return *this;
}

bool PreparedStatement::bindParameter(int index, const Symbols::ValuePtr& value) {
    if (!is_prepared_ && !prepare()) {
        return false;
    }
    
    validateParameterIndex(index);
    
    if (!SecurityValidator::isValidParameterType(value)) {
        throw SecurityException("Invalid parameter type for binding at index " + std::to_string(index));
    }
    
    return bindParameterByType(index, value);
}

bool PreparedStatement::bindParameters(const std::vector<Symbols::ValuePtr>& values) {
    if (!is_prepared_ && !prepare()) {
        return false;
    }
    
    if (static_cast<int>(values.size()) != parameter_count_) {
        throw SecurityException("Parameter count mismatch. Expected: " + std::to_string(parameter_count_) +
                               ", got: " + std::to_string(values.size()));
    }
    
    // Validate all parameters first
    SecurityValidator::validateParameters(values);
    
    // Bind all parameters
    for (size_t i = 0; i < values.size(); ++i) {
        if (!bindParameter(static_cast<int>(i), values[i])) {
            return false;
        }
    }
    
    return true;
}

void PreparedStatement::clearParameters() {
    string_params_.clear();
    int_params_.clear();
    double_params_.clear();
    bool_params_.clear();
    param_lengths_.clear();
    param_binds_.clear();
}

bool PreparedStatement::execute() {
    if (!is_prepared_) {
        throw SecurityException("Statement not prepared");
    }
    
    if (!stmt_) {
        throw SecurityException("Invalid statement handle");
    }
    
    
    if (mysql_stmt_execute(stmt_) != 0) {
        std::string error = mysql_stmt_error(stmt_);
        throw QueryException("Failed to execute prepared statement: " + error, mysql_stmt_errno(stmt_));
    }
    
    return true;
}

Symbols::ValuePtr PreparedStatement::executeQuery() {
    if (!execute()) {
        return Symbols::ValuePtr::null();
    }
    
    // For SELECT queries, process results
    if (SecurityValidator::isSelectQuery(query_)) {
        // Store result
        if (mysql_stmt_store_result(stmt_) != 0) {
            std::string error = mysql_stmt_error(stmt_);
            throw QueryException("Failed to store result: " + error, mysql_stmt_errno(stmt_));
        }
        
        // Get result metadata
        MYSQL_RES* metadata = mysql_stmt_result_metadata(stmt_);
        if (!metadata) {
            return Symbols::ValuePtr::null();
        }
        
        unsigned int num_fields = mysql_num_fields(metadata);
        std::vector<std::string> field_names;
        
        // Get field names
        for (unsigned int i = 0; i < num_fields; ++i) {
            MYSQL_FIELD* field = mysql_fetch_field_direct(metadata, i);
            field_names.emplace_back(field->name);
        }
        
        mysql_free_result(metadata);
        
        // Build result object with proper result processing
        Symbols::ObjectMap result;
        
        // Prepare result binding for proper data retrieval
        std::vector<MYSQL_BIND> result_binds(num_fields);
        std::vector<std::string> string_buffers(num_fields);
        std::vector<unsigned long> lengths(num_fields);
        std::vector<my_bool> is_null_flags(num_fields);
        
        // Initialize result bindings
        for (unsigned int i = 0; i < num_fields; ++i) {
            MYSQL_FIELD* field = mysql_fetch_field_direct(metadata, i);
            
            memset(&result_binds[i], 0, sizeof(MYSQL_BIND));
            
            // Set up string buffer for all types (will convert as needed)
            string_buffers[i].resize(1024); // Reasonable buffer size
            result_binds[i].buffer_type = MYSQL_TYPE_STRING;
            result_binds[i].buffer = &string_buffers[i][0];
            result_binds[i].buffer_length = string_buffers[i].size();
            result_binds[i].length = &lengths[i];
            result_binds[i].is_null = &is_null_flags[i];
        }
        
        // Bind result columns
        if (mysql_stmt_bind_result(stmt_, result_binds.data()) != 0) {
            mysql_free_result(metadata);
            throw QueryException("Failed to bind result columns: " + std::string(mysql_stmt_error(stmt_)));
        }
        
        // Fetch and process results
        int row_index = 0;
        while (mysql_stmt_fetch(stmt_) == 0) {
            Symbols::ObjectMap row_data;
            
            for (unsigned int i = 0; i < num_fields; ++i) {
                if (is_null_flags[i]) {
                    row_data[field_names[i]] = Symbols::ValuePtr::null();
                } else {
                    // Resize string to actual length and create value
                    string_buffers[i].resize(lengths[i]);
                    row_data[field_names[i]] = Symbols::ValuePtr(string_buffers[i]);
                    string_buffers[i].resize(1024); // Reset buffer size for next row
                }
            }
            
            result[std::to_string(row_index++)] = Symbols::ValuePtr(row_data);
        }
        
        return Symbols::ValuePtr(result);
    }
    
    return Symbols::ValuePtr::null();
}

int PreparedStatement::executeUpdate() {
    if (!execute()) {
        return -1;
    }
    
    return static_cast<int>(mysql_stmt_affected_rows(stmt_));
}

bool PreparedStatement::fetch() {
    if (!stmt_) {
        return false;
    }
    
    int result = mysql_stmt_fetch(stmt_);
    return result == 0;
}

Symbols::ValuePtr PreparedStatement::getResult() {
    if (!stmt_) {
        return Symbols::ValuePtr::null();
    }
    
    // Get result metadata
    MYSQL_RES* metadata = mysql_stmt_result_metadata(stmt_);
    if (!metadata) {
        return Symbols::ValuePtr::null();
    }
    
    unsigned int num_fields = mysql_num_fields(metadata);
    if (num_fields == 0) {
        mysql_free_result(metadata);
        return Symbols::ValuePtr::null();
    }
    
    // Build single row result for current row
    std::vector<std::string> field_names;
    for (unsigned int i = 0; i < num_fields; ++i) {
        MYSQL_FIELD* field = mysql_fetch_field_direct(metadata, i);
        field_names.emplace_back(field->name);
    }
    
    // Set up result binding for current row
    std::vector<MYSQL_BIND> result_binds(num_fields);
    std::vector<std::string> string_buffers(num_fields);
    std::vector<unsigned long> lengths(num_fields);
    std::vector<my_bool> is_null_flags(num_fields);
    
    for (unsigned int i = 0; i < num_fields; ++i) {
        string_buffers[i].resize(1024);
        memset(&result_binds[i], 0, sizeof(MYSQL_BIND));
        result_binds[i].buffer_type = MYSQL_TYPE_STRING;
        result_binds[i].buffer = &string_buffers[i][0];
        result_binds[i].buffer_length = string_buffers[i].size();
        result_binds[i].length = &lengths[i];
        result_binds[i].is_null = &is_null_flags[i];
    }
    
    // Bind result columns
    if (mysql_stmt_bind_result(stmt_, result_binds.data()) != 0) {
        mysql_free_result(metadata);
        return Symbols::ValuePtr::null();
    }
    
    // Fetch current row data
    if (mysql_stmt_fetch(stmt_) == 0) {
        Symbols::ObjectMap row_data;
        
        for (unsigned int i = 0; i < num_fields; ++i) {
            if (is_null_flags[i]) {
                row_data[field_names[i]] = Symbols::ValuePtr::null();
            } else {
                string_buffers[i].resize(lengths[i]);
                row_data[field_names[i]] = Symbols::ValuePtr(string_buffers[i]);
            }
        }
        
        mysql_free_result(metadata);
        return Symbols::ValuePtr(row_data);
    }
    
    mysql_free_result(metadata);
    return Symbols::ValuePtr::null();
}

int PreparedStatement::getAffectedRows() {
    if (!stmt_) {
        return -1;
    }
    
    return static_cast<int>(mysql_stmt_affected_rows(stmt_));
}

uint64_t PreparedStatement::getLastInsertId() {
    if (!stmt_) {
        return 0;
    }
    
    return mysql_stmt_insert_id(stmt_);
}

bool PreparedStatement::prepare() {
    if (!connection_ || !connection_->isConnected()) {
        throw ConnectionException("No valid connection for prepared statement");
    }
    
    MYSQL* handle = connection_->getHandle();
    if (!handle) {
        throw ConnectionException("Invalid MySQL handle");
    }
    
    stmt_ = mysql_stmt_init(handle);
    if (!stmt_) {
        throw QueryException("Failed to initialize prepared statement");
    }
    
    if (mysql_stmt_prepare(stmt_, query_.c_str(), query_.length()) != 0) {
        std::string error = mysql_stmt_error(stmt_);
        cleanup();
        throw QueryException("Failed to prepare statement: " + error, mysql_stmt_errno(stmt_));
    }
    
    parameter_count_ = mysql_stmt_param_count(stmt_);
    is_prepared_ = true;
    
    
    return true;
}

void PreparedStatement::setupParameterBinds() {
    if (parameter_count_ > 0) {
        param_binds_.resize(parameter_count_);
        memset(param_binds_.data(), 0, sizeof(MYSQL_BIND) * parameter_count_);
    }
}

void PreparedStatement::setupResultBinds() {
    // Result binding setup for Phase 2
    // Full implementation would be added in later phases
}

void PreparedStatement::cleanup() {
    if (stmt_) {
        mysql_stmt_close(stmt_);
        stmt_ = nullptr;
    }
    
    is_prepared_ = false;
    parameter_count_ = 0;
    clearParameters();
}

bool PreparedStatement::bindParameterByType(int index, const Symbols::ValuePtr& value) {
    if (param_binds_.size() != static_cast<size_t>(parameter_count_)) {
        setupParameterBinds();
    }
    
    Symbols::Variables::Type type = value.getType();
    MYSQL_BIND& bind = param_binds_[index];
    
    // Reset bind structure
    memset(&bind, 0, sizeof(MYSQL_BIND));
    
    switch (type) {
        case Symbols::Variables::Type::STRING: {
            if (index >= static_cast<int>(string_params_.size())) {
                string_params_.resize(index + 1);
                param_lengths_.resize(index + 1);
            }
            
            std::string str_val = value.get<std::string>();
            string_params_[index] = str_val;
            param_lengths_[index] = str_val.length();
            
            bind.buffer_type = MYSQL_TYPE_STRING;
            bind.buffer = const_cast<char*>(string_params_[index].c_str());
            bind.buffer_length = param_lengths_[index];
            bind.length = &param_lengths_[index];
            break;
        }
        
        case Symbols::Variables::Type::INTEGER: {
            if (index >= static_cast<int>(int_params_.size())) {
                int_params_.resize(index + 1);
            }
            
            int_params_[index] = value.get<int>();
            
            bind.buffer_type = MYSQL_TYPE_LONG;
            bind.buffer = &int_params_[index];
            bind.buffer_length = sizeof(int);
            break;
        }
        
        case Symbols::Variables::Type::DOUBLE: {
            if (index >= static_cast<int>(double_params_.size())) {
                double_params_.resize(index + 1);
            }
            
            double_params_[index] = value.get<double>();
            
            bind.buffer_type = MYSQL_TYPE_DOUBLE;
            bind.buffer = &double_params_[index];
            bind.buffer_length = sizeof(double);
            break;
        }
        
        case Symbols::Variables::Type::BOOLEAN: {
            if (index >= static_cast<int>(bool_params_.size())) {
                bool_params_.resize(index + 1);
            }
            
            bool_params_[index] = value.get<bool>() ? 1 : 0;

            bind.buffer_type = MYSQL_TYPE_TINY;
            bind.buffer = &bool_params_[index];
            bind.buffer_length = sizeof(char);
            break;
        }
        
        case Symbols::Variables::Type::NULL_TYPE: {
            bind.buffer_type = MYSQL_TYPE_NULL;
            bind.is_null = new my_bool(1);
            break;
        }
        
        default:
            throw SecurityException("Unsupported parameter type for binding");
    }
    
    // Bind the parameter
    if (mysql_stmt_bind_param(stmt_, param_binds_.data()) != 0) {
        std::string error = mysql_stmt_error(stmt_);
        throw QueryException("Failed to bind parameters: " + error, mysql_stmt_errno(stmt_));
    }
    
    return true;
}

void PreparedStatement::validateParameterIndex(int index) {
    if (index < 0 || index >= parameter_count_) {
        throw SecurityException("Parameter index out of range: " + std::to_string(index) +
                               " (valid range: 0-" + std::to_string(parameter_count_ - 1) + ")");
    }
}

//=============================================================================
// QueryBuilder Implementation (Phase 2)
//=============================================================================

QueryBuilder::QueryBuilder()
    : limit_count_(0)
    , offset_count_(0)
    , has_limit_(false)
    , has_offset_(false)
{
}

QueryBuilder& QueryBuilder::select(const std::vector<std::string>& columns) {
    validateColumnNames(columns);
    select_columns_ = columns;
    return *this;
}

QueryBuilder& QueryBuilder::select(const std::string& column) {
    validateColumnName(column);
    select_columns_.clear();
    select_columns_.push_back(column);
    return *this;
}

QueryBuilder& QueryBuilder::from(const std::string& table) {
    validateTableName(table);
    table_name_ = table;
    return *this;
}

QueryBuilder& QueryBuilder::where(const std::string& condition) {
    if (condition.empty()) {
        throw SecurityException("WHERE condition cannot be empty");
    }
    
    // Basic validation - check for potential injection
    if (SecurityValidator::containsSQLInjection(condition)) {
        throw SecurityException("WHERE condition contains potential SQL injection");
    }
    
    where_conditions_.push_back(condition);
    return *this;
}

QueryBuilder& QueryBuilder::whereEquals(const std::string& column, const Symbols::ValuePtr& value) {
    validateColumnName(column);
    
    if (!SecurityValidator::isValidParameterType(value)) {
        throw SecurityException("Invalid parameter type for WHERE condition");
    }
    
    where_conditions_.push_back(column + " = ?");
    parameters_.push_back(value);
    return *this;
}

QueryBuilder& QueryBuilder::orderBy(const std::string& column, bool ascending) {
    validateColumnName(column);
    
    std::string order_clause = column + (ascending ? " ASC" : " DESC");
    order_by_columns_.push_back(order_clause);
    return *this;
}

QueryBuilder& QueryBuilder::limit(int count, int offset) {
    if (count < 0) {
        throw SecurityException("LIMIT count cannot be negative");
    }
    
    if (offset < 0) {
        throw SecurityException("LIMIT offset cannot be negative");
    }
    
    limit_count_ = count;
    offset_count_ = offset;
    has_limit_ = true;
    has_offset_ = (offset > 0);
    return *this;
}

QueryBuilder& QueryBuilder::bindParameter(const std::string& name, const Symbols::ValuePtr& value) {
    if (name.empty()) {
        throw SecurityException("Parameter name cannot be empty");
    }
    
    if (!SecurityValidator::isValidParameterType(value)) {
        throw SecurityException("Invalid parameter type for binding: " + name);
    }
    
    named_parameters_[name] = value;
    return *this;
}

QueryBuilder& QueryBuilder::bindParameters(const std::vector<Symbols::ValuePtr>& values) {
    SecurityValidator::validateParameters(values);
    parameters_ = values;
    return *this;
}

std::string QueryBuilder::buildQuery() {
    validate();
    
    if (select_columns_.empty() || table_name_.empty()) {
        throw SecurityException("Invalid query: missing SELECT columns or FROM table");
    }
    
    std::stringstream query;
    
    // Build SELECT clause
    query << "SELECT ";
    for (size_t i = 0; i < select_columns_.size(); ++i) {
        if (i > 0) query << ", ";
        query << "`" << select_columns_[i] << "`";
    }
    
    // Build FROM clause
    query << " FROM `" << table_name_ << "`";
    
    // Build WHERE clause
    if (!where_conditions_.empty()) {
        query << buildWhereClause();
    }
    
    // Build ORDER BY clause
    if (!order_by_columns_.empty()) {
        query << buildOrderByClause();
    }
    
    // Build LIMIT clause
    if (has_limit_) {
        query << buildLimitClause();
    }
    
    std::string result = query.str();
    
    
    return result;
}

std::vector<Symbols::ValuePtr> QueryBuilder::getParameters() const {
    return parameters_;
}

std::string QueryBuilder::buildInsertQuery(const std::map<std::string, Symbols::ValuePtr>& data) {
    if (table_name_.empty()) {
        throw SecurityException("Table name not specified for INSERT query");
    }
    
    if (data.empty()) {
        throw SecurityException("No data provided for INSERT query");
    }
    
    validateTableName(table_name_);
    
    std::stringstream query;
    std::vector<std::string> columns;
    std::vector<Symbols::ValuePtr> values;
    
    query << "INSERT INTO `" << table_name_ << "` (";
    
    bool first = true;
    for (const auto& [column, value] : data) {
        validateColumnName(column);
        
        if (!SecurityValidator::isValidParameterType(value)) {
            throw SecurityException("Invalid parameter type for column: " + column);
        }
        
        if (!first) query << ", ";
        query << "`" << column << "`";
        
        columns.push_back(column);
        values.push_back(value);
        first = false;
    }
    
    query << ") VALUES (";
    
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) query << ", ";
        query << "?";
    }
    
    query << ")";
    
    // Store parameters for later binding
    parameters_ = values;
    
    std::string result = query.str();
    
    return result;
}

std::string QueryBuilder::buildUpdateQuery(const std::map<std::string, Symbols::ValuePtr>& data,
                                          const std::map<std::string, Symbols::ValuePtr>& conditions) {
    if (table_name_.empty()) {
        throw SecurityException("Table name not specified for UPDATE query");
    }
    
    if (data.empty()) {
        throw SecurityException("No data provided for UPDATE query");
    }
    
    if (conditions.empty()) {
        throw SecurityException("No conditions provided for UPDATE query - this would update all rows");
    }
    
    validateTableName(table_name_);
    
    std::stringstream query;
    std::vector<Symbols::ValuePtr> values;
    
    query << "UPDATE `" << table_name_ << "` SET ";
    
    bool first = true;
    for (const auto& [column, value] : data) {
        validateColumnName(column);
        
        if (!SecurityValidator::isValidParameterType(value)) {
            throw SecurityException("Invalid parameter type for column: " + column);
        }
        
        if (!first) query << ", ";
        query << "`" << column << "` = ?";
        
        values.push_back(value);
        first = false;
    }
    
    query << " WHERE ";
    
    first = true;
    for (const auto& [column, value] : conditions) {
        validateColumnName(column);
        
        if (!SecurityValidator::isValidParameterType(value)) {
            throw SecurityException("Invalid parameter type for condition column: " + column);
        }
        
        if (!first) query << " AND ";
        query << "`" << column << "` = ?";
        
        values.push_back(value);
        first = false;
    }
    
    // Store parameters for later binding
    parameters_ = values;
    
    std::string result = query.str();
    
    return result;
}

std::string QueryBuilder::buildDeleteQuery(const std::map<std::string, Symbols::ValuePtr>& conditions) {
    if (table_name_.empty()) {
        throw SecurityException("Table name not specified for DELETE query");
    }
    
    if (conditions.empty()) {
        throw SecurityException("No conditions provided for DELETE query - this would delete all rows");
    }
    
    validateTableName(table_name_);
    
    std::stringstream query;
    std::vector<Symbols::ValuePtr> values;
    
    query << "DELETE FROM `" << table_name_ << "` WHERE ";
    
    bool first = true;
    for (const auto& [column, value] : conditions) {
        validateColumnName(column);
        
        if (!SecurityValidator::isValidParameterType(value)) {
            throw SecurityException("Invalid parameter type for condition column: " + column);
        }
        
        if (!first) query << " AND ";
        query << "`" << column << "` = ?";
        
        values.push_back(value);
        first = false;
    }
    
    // Store parameters for later binding
    parameters_ = values;
    
    std::string result = query.str();
    
    return result;
}

void QueryBuilder::validate() {
    if (!table_name_.empty()) {
        validateTableName(table_name_);
    }
    
    for (const auto& column : select_columns_) {
        validateColumnName(column);
    }
    
    // Validate parameters
    SecurityValidator::validateParameters(parameters_);
}

void QueryBuilder::reset() {
    base_query_.clear();
    table_name_.clear();
    select_columns_.clear();
    where_conditions_.clear();
    order_by_columns_.clear();
    parameters_.clear();
    named_parameters_.clear();
    limit_count_ = 0;
    offset_count_ = 0;
    has_limit_ = false;
    has_offset_ = false;
}

void QueryBuilder::validateTableName(const std::string& table) {
    if (!SecurityValidator::validateTableName(table)) {
        throw SecurityException("Invalid table name: " + table);
    }
}

void QueryBuilder::validateColumnNames(const std::vector<std::string>& columns) {
    for (const auto& column : columns) {
        validateColumnName(column);
    }
}

void QueryBuilder::validateColumnName(const std::string& column) {
    if (!SecurityValidator::validateColumnName(column)) {
        throw SecurityException("Invalid column name: " + column);
    }
}

std::string QueryBuilder::buildWhereClause() {
    std::stringstream clause;
    clause << " WHERE ";
    
    for (size_t i = 0; i < where_conditions_.size(); ++i) {
        if (i > 0) clause << " AND ";
        clause << "(" << where_conditions_[i] << ")";
    }
    
    return clause.str();
}

std::string QueryBuilder::buildOrderByClause() {
    std::stringstream clause;
    clause << " ORDER BY ";
    
    for (size_t i = 0; i < order_by_columns_.size(); ++i) {
        if (i > 0) clause << ", ";
        clause << order_by_columns_[i];
    }
    
    return clause.str();
}

std::string QueryBuilder::buildLimitClause() {
    std::stringstream clause;
    clause << " LIMIT " << limit_count_;
    
    if (has_offset_) {
        clause << " OFFSET " << offset_count_;
    }
    
    return clause.str();
}

//=============================================================================
// MariaDBModule Implementation
//=============================================================================

MariaDBModule::MariaDBModule()
    : connection_manager_(std::make_unique<ConnectionManager>())
    , query_executor_(std::make_unique<QueryExecutor>(connection_manager_.get()))
{
    this->setModuleName("MariaDB");
    this->setDescription("Provides comprehensive database connectivity and operations for MariaDB/MySQL databases including connection management, query execution, transactions, and prepared statements with advanced security features");
    initializeModule();
    initializeSecurityFramework();
    initializeQueryExecutor();
    initializeTransactionManager();
}

MariaDBModule::~MariaDBModule() {
    cleanupTransactionResources();
    cleanupSecurityResources();
    cleanupConnections();
}

void MariaDBModule::registerFunctions() {
    
    // Register MariaDB class
    REGISTER_CLASS("MariaDB");

    // Constructor - new MariaDB()
    std::vector<Symbols::FunctionParameterInfo> constructor_params = {};
    REGISTER_METHOD("MariaDB", "__construct", constructor_params,
                    [this](FunctionArguments& args) -> Symbols::ValuePtr {
                        if (args.size() != 1) {
                            throw std::runtime_error("MariaDB::__construct expects no parameters, got: " + std::to_string(args.size() - 1));
                        }
                        
                        if (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT) {
                            throw std::runtime_error("MariaDB::__construct must be called on MariaDB instance");
                        }
                        
                        // Return the original object - it's already initialized
                        return args[0];
                    },
                    Symbols::Variables::Type::CLASS,
                    "Create new MariaDB instance");

    // Enhanced connect method with improved parameter handling
    std::vector<Symbols::FunctionParameterInfo> params = {
        { "host", Symbols::Variables::Type::STRING, "Database host to connect", false },
        { "user", Symbols::Variables::Type::STRING, "Username to authenticate", false },
        { "pass", Symbols::Variables::Type::STRING, "Password to authenticate", false },
        { "db",   Symbols::Variables::Type::STRING, "Database name", false },
    };

    REGISTER_METHOD(
        "MariaDB", "connect", params, [this](FunctionArguments& args) { return this->connect(args); },
        Symbols::Variables::Type::CLASS, "Connect to MariaDB host with enhanced connection management");

    // Enhanced query method
    params = {
        { "query_string", Symbols::Variables::Type::STRING, "SQL query string to execute", false },
    };

    REGISTER_METHOD(
        "MariaDB", "query", params, [this](FunctionArguments& args) { return this->query(args); },
        Symbols::Variables::Type::OBJECT, "Execute MariaDB query with improved error handling");

    // Enhanced connection management methods
    REGISTER_METHOD(
        "MariaDB", "disconnect", {}, [this](FunctionArguments& args) { return this->disconnect(args); },
        Symbols::Variables::Type::NULL_TYPE, "Disconnect from MariaDB with proper cleanup");
        
    REGISTER_METHOD(
        "MariaDB", "close", {}, [this](FunctionArguments& args) { return this->close(args); },
        Symbols::Variables::Type::NULL_TYPE, "Close MariaDB connection (alias for disconnect)");

    REGISTER_METHOD(
        "MariaDB", "isConnected", {}, [this](FunctionArguments& args) { return this->isConnected(args); },
        Symbols::Variables::Type::BOOLEAN, "Check if connection is active");

    REGISTER_METHOD(
        "MariaDB", "reconnect", {}, [this](FunctionArguments& args) { return this->reconnect(args); },
        Symbols::Variables::Type::BOOLEAN, "Reconnect to database");

    // Utility methods
    params = {
        { "input", Symbols::Variables::Type::STRING, "String to escape", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "escapeString", params, [this](FunctionArguments& args) { return this->escapeString(args); },
        Symbols::Variables::Type::STRING, "Escape string for safe SQL usage");

    REGISTER_METHOD(
        "MariaDB", "getLastInsertId", {}, [this](FunctionArguments& args) { return this->getLastInsertId(args); },
        Symbols::Variables::Type::INTEGER, "Get last inserted row ID");

    REGISTER_METHOD(
        "MariaDB", "getAffectedRows", {}, [this](FunctionArguments& args) { return this->getAffectedRows(args); },
        Symbols::Variables::Type::INTEGER, "Get number of affected rows");

    REGISTER_METHOD(
        "MariaDB", "getConnectionInfo", {}, [this](FunctionArguments& args) { return this->getConnectionInfo(args); },
        Symbols::Variables::Type::OBJECT, "Get connection information and status");

    
    // Input validation method
    params = {
        { "input", Symbols::Variables::Type::STRING, "Input string to validate", false },
        { "type", Symbols::Variables::Type::STRING, "Type of validation to perform", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "validateInput", params, [this](FunctionArguments& args) { return this->validateInput(args); },
        Symbols::Variables::Type::BOOLEAN, "Validate input for security and type compliance");

    // Prepared statement methods
    params = {
        { "query", Symbols::Variables::Type::STRING, "SQL query with parameter placeholders", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "prepareStatement", params, [this](FunctionArguments& args) { return this->prepareStatement(args); },
        Symbols::Variables::Type::STRING, "Prepare SQL statement for safe execution");

    params = {
        { "stmt_key", Symbols::Variables::Type::STRING, "Prepared statement key", false },
        { "index", Symbols::Variables::Type::INTEGER, "Parameter index (0-based)", false },
        { "value", Symbols::Variables::Type::STRING, "Parameter value", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "bindParameter", params, [this](FunctionArguments& args) { return this->bindParameter(args); },
        Symbols::Variables::Type::BOOLEAN, "Bind parameter to prepared statement");

    params = {
        { "query", Symbols::Variables::Type::STRING, "SQL query with parameter placeholders", false },
        { "parameters", Symbols::Variables::Type::OBJECT, "Array of parameters to bind", true },
    };
    
    REGISTER_METHOD(
        "MariaDB", "executeQuery", params, [this](FunctionArguments& args) { return this->executeQuery(args); },
        Symbols::Variables::Type::OBJECT, "Execute parameterized query safely");

    params = {
        { "stmt_key", Symbols::Variables::Type::STRING, "Prepared statement key", false },
        { "parameters", Symbols::Variables::Type::OBJECT, "Array of parameters to bind", true },
    };
    
    REGISTER_METHOD(
        "MariaDB", "executePrepared", params, [this](FunctionArguments& args) { return this->executePrepared(args); },
        Symbols::Variables::Type::OBJECT, "Execute prepared statement with parameters");

    // Query builder methods
    params = {
        { "table", Symbols::Variables::Type::STRING, "Table name", false },
        { "columns", Symbols::Variables::Type::OBJECT, "Array of column names", true },
        { "conditions", Symbols::Variables::Type::OBJECT, "WHERE conditions object", true },
    };
    
    REGISTER_METHOD(
        "MariaDB", "buildSelectQuery", params, [this](FunctionArguments& args) { return this->buildSelectQuery(args); },
        Symbols::Variables::Type::STRING, "Build safe SELECT query");

    params = {
        { "table", Symbols::Variables::Type::STRING, "Table name", false },
        { "data", Symbols::Variables::Type::OBJECT, "Column-value pairs to insert", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "buildInsertQuery", params, [this](FunctionArguments& args) { return this->buildInsertQuery(args); },
        Symbols::Variables::Type::STRING, "Build safe INSERT query");

    params = {
        { "table", Symbols::Variables::Type::STRING, "Table name", false },
        { "data", Symbols::Variables::Type::OBJECT, "Column-value pairs to update", false },
        { "conditions", Symbols::Variables::Type::OBJECT, "WHERE conditions object", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "buildUpdateQuery", params, [this](FunctionArguments& args) { return this->buildUpdateQuery(args); },
        Symbols::Variables::Type::STRING, "Build safe UPDATE query");

    params = {
        { "table", Symbols::Variables::Type::STRING, "Table name", false },
        { "conditions", Symbols::Variables::Type::OBJECT, "WHERE conditions object", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "buildDeleteQuery", params, [this](FunctionArguments& args) { return this->buildDeleteQuery(args); },
        Symbols::Variables::Type::STRING, "Build safe DELETE query");
    
    // SELECT operations
    params = {
        { "table", Symbols::Variables::Type::STRING, "Table name", false },
        { "columns", Symbols::Variables::Type::OBJECT, "Array of column names", true },
        { "conditions", Symbols::Variables::Type::OBJECT, "WHERE conditions object", true },
        { "orderBy", Symbols::Variables::Type::STRING, "ORDER BY clause", true },
        { "limit", Symbols::Variables::Type::INTEGER, "LIMIT count", true },
        { "offset", Symbols::Variables::Type::INTEGER, "OFFSET count", true },
    };
    
    REGISTER_METHOD(
        "MariaDB", "select", params, [this](FunctionArguments& args) { return this->select(args); },
        Symbols::Variables::Type::OBJECT, "Execute SELECT query with advanced options");

    params = {
        { "table", Symbols::Variables::Type::STRING, "Table name", false },
        { "columns", Symbols::Variables::Type::OBJECT, "Array of column names", true },
        { "conditions", Symbols::Variables::Type::OBJECT, "WHERE conditions object", true },
    };
    
    REGISTER_METHOD(
        "MariaDB", "selectOne", params, [this](FunctionArguments& args) { return this->selectOne(args); },
        Symbols::Variables::Type::OBJECT, "Select single row");

    params = {
        { "table", Symbols::Variables::Type::STRING, "Table name", false },
        { "column", Symbols::Variables::Type::STRING, "Column name", false },
        { "conditions", Symbols::Variables::Type::OBJECT, "WHERE conditions object", true },
    };
    
    REGISTER_METHOD(
        "MariaDB", "selectColumn", params, [this](FunctionArguments& args) { return this->selectColumn(args); },
        Symbols::Variables::Type::STRING, "Select single column value");

    REGISTER_METHOD(
        "MariaDB", "selectScalar", params, [this](FunctionArguments& args) { return this->selectScalar(args); },
        Symbols::Variables::Type::STRING, "Select single scalar value");

    // INSERT operations
    params = {
        { "table", Symbols::Variables::Type::STRING, "Table name", false },
        { "data", Symbols::Variables::Type::OBJECT, "Column-value pairs to insert", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "insert", params, [this](FunctionArguments& args) { return this->insert(args); },
        Symbols::Variables::Type::INTEGER, "Insert single record");

    params = {
        { "table", Symbols::Variables::Type::STRING, "Table name", false },
        { "dataArray", Symbols::Variables::Type::OBJECT, "Array of data objects to insert", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "insertBatch", params, [this](FunctionArguments& args) { return this->insertBatch(args); },
        Symbols::Variables::Type::OBJECT, "Insert multiple records in batch");

    params = {
        { "table", Symbols::Variables::Type::STRING, "Table name", false },
        { "data", Symbols::Variables::Type::OBJECT, "Column-value pairs to insert", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "insertAndGetId", params, [this](FunctionArguments& args) { return this->insertAndGetId(args); },
        Symbols::Variables::Type::INTEGER, "Insert record and return auto-increment ID");

    // UPDATE operations
    params = {
        { "table", Symbols::Variables::Type::STRING, "Table name", false },
        { "data", Symbols::Variables::Type::OBJECT, "Column-value pairs to update", false },
        { "conditions", Symbols::Variables::Type::OBJECT, "WHERE conditions object", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "update", params, [this](FunctionArguments& args) { return this->update(args); },
        Symbols::Variables::Type::INTEGER, "Update records with conditions");

    params = {
        { "table", Symbols::Variables::Type::STRING, "Table name", false },
        { "dataArray", Symbols::Variables::Type::OBJECT, "Array of data objects to update", false },
        { "keyColumn", Symbols::Variables::Type::STRING, "Key column for updates", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "updateBatch", params, [this](FunctionArguments& args) { return this->updateBatch(args); },
        Symbols::Variables::Type::OBJECT, "Update multiple records in batch");

    // DELETE operations
    params = {
        { "table", Symbols::Variables::Type::STRING, "Table name", false },
        { "conditions", Symbols::Variables::Type::OBJECT, "WHERE conditions object", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "deleteRecord", params, [this](FunctionArguments& args) { return this->deleteRecord(args); },
        Symbols::Variables::Type::INTEGER, "Delete records with conditions");

    params = {
        { "table", Symbols::Variables::Type::STRING, "Table name", false },
        { "keyValues", Symbols::Variables::Type::OBJECT, "Array of key values to delete", false },
        { "keyColumn", Symbols::Variables::Type::STRING, "Key column for deletes", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "deleteBatch", params, [this](FunctionArguments& args) { return this->deleteBatch(args); },
        Symbols::Variables::Type::OBJECT, "Delete multiple records in batch");

    // Schema operations
    params = {
        { "tableName", Symbols::Variables::Type::STRING, "Table name", false },
        { "columns", Symbols::Variables::Type::OBJECT, "Column definitions object", false },
        { "constraints", Symbols::Variables::Type::OBJECT, "Table constraints array", true },
    };
    
    REGISTER_METHOD(
        "MariaDB", "createTable", params, [this](FunctionArguments& args) { return this->createTable(args); },
        Symbols::Variables::Type::BOOLEAN, "Create new table");

    params = {
        { "tableName", Symbols::Variables::Type::STRING, "Table name", false },
        { "ifExists", Symbols::Variables::Type::BOOLEAN, "Add IF EXISTS clause", true },
    };
    
    REGISTER_METHOD(
        "MariaDB", "dropTable", params, [this](FunctionArguments& args) { return this->dropTable(args); },
        Symbols::Variables::Type::BOOLEAN, "Drop table");

    params = {
        { "tableName", Symbols::Variables::Type::STRING, "Table name", false },
        { "columns", Symbols::Variables::Type::OBJECT, "Array of column names", false },
        { "indexName", Symbols::Variables::Type::STRING, "Index name", true },
        { "unique", Symbols::Variables::Type::BOOLEAN, "Create unique index", true },
    };
    
    REGISTER_METHOD(
        "MariaDB", "createIndex", params, [this](FunctionArguments& args) { return this->createIndex(args); },
        Symbols::Variables::Type::BOOLEAN, "Create index");

    params = {
        { "tableName", Symbols::Variables::Type::STRING, "Table name", false },
        { "indexName", Symbols::Variables::Type::STRING, "Index name", false },
        { "ifExists", Symbols::Variables::Type::BOOLEAN, "Add IF EXISTS clause", true },
    };
    
    REGISTER_METHOD(
        "MariaDB", "dropIndex", params, [this](FunctionArguments& args) { return this->dropIndex(args); },
        Symbols::Variables::Type::BOOLEAN, "Drop index");

    // Utility operations
    params = {
        { "table", Symbols::Variables::Type::STRING, "Table name", false },
        { "conditions", Symbols::Variables::Type::OBJECT, "WHERE conditions object", true },
    };
    
    REGISTER_METHOD(
        "MariaDB", "getRowCount", params, [this](FunctionArguments& args) { return this->getRowCount(args); },
        Symbols::Variables::Type::INTEGER, "Get row count with optional conditions");

   
    // Basic transaction control
    REGISTER_METHOD(
        "MariaDB", "beginTransaction", {}, [this](FunctionArguments& args) { return this->beginTransaction(args); },
        Symbols::Variables::Type::BOOLEAN, "Begin a new transaction");

    REGISTER_METHOD(
        "MariaDB", "commitTransaction", {}, [this](FunctionArguments& args) { return this->commitTransaction(args); },
        Symbols::Variables::Type::BOOLEAN, "Commit current transaction");

    REGISTER_METHOD(
        "MariaDB", "rollbackTransaction", {}, [this](FunctionArguments& args) { return this->rollbackTransaction(args); },
        Symbols::Variables::Type::BOOLEAN, "Rollback current transaction");

    REGISTER_METHOD(
        "MariaDB", "isInTransaction", {}, [this](FunctionArguments& args) { return this->isInTransaction(args); },
        Symbols::Variables::Type::BOOLEAN, "Check if currently in a transaction");

    // Savepoint management
    params = {
        { "name", Symbols::Variables::Type::STRING, "Savepoint name", true },
    };
    
    REGISTER_METHOD(
        "MariaDB", "createSavepoint", params, [this](FunctionArguments& args) { return this->createSavepoint(args); },
        Symbols::Variables::Type::STRING, "Create a named savepoint");

    params = {
        { "name", Symbols::Variables::Type::STRING, "Savepoint name", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "rollbackToSavepoint", params, [this](FunctionArguments& args) { return this->rollbackToSavepoint(args); },
        Symbols::Variables::Type::BOOLEAN, "Rollback to a named savepoint");

    REGISTER_METHOD(
        "MariaDB", "releaseSavepoint", params, [this](FunctionArguments& args) { return this->releaseSavepoint(args); },
        Symbols::Variables::Type::BOOLEAN, "Release a named savepoint");

    // Isolation level management
    params = {
        { "level", Symbols::Variables::Type::STRING, "Isolation level (READ_UNCOMMITTED, READ_COMMITTED, REPEATABLE_READ, SERIALIZABLE)", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "setIsolationLevel", params, [this](FunctionArguments& args) { return this->setIsolationLevel(args); },
        Symbols::Variables::Type::BOOLEAN, "Set transaction isolation level");

    REGISTER_METHOD(
        "MariaDB", "getIsolationLevel", {}, [this](FunctionArguments& args) { return this->getIsolationLevel(args); },
        Symbols::Variables::Type::STRING, "Get current transaction isolation level");

    // Auto-commit control
    params = {
        { "enabled", Symbols::Variables::Type::BOOLEAN, "Enable or disable auto-commit", false },
    };
    
    REGISTER_METHOD(
        "MariaDB", "setAutoCommit", params, [this](FunctionArguments& args) { return this->setAutoCommit(args); },
        Symbols::Variables::Type::BOOLEAN, "Set auto-commit mode");

    REGISTER_METHOD(
        "MariaDB", "getAutoCommit", {}, [this](FunctionArguments& args) { return this->getAutoCommit(args); },
        Symbols::Variables::Type::BOOLEAN, "Get auto-commit status");

    // Advanced transaction features
    REGISTER_METHOD(
        "MariaDB", "detectDeadlock", {}, [this](FunctionArguments& args) { return this->detectDeadlock(args); },
        Symbols::Variables::Type::BOOLEAN, "Detect if current transaction is in deadlock");

    REGISTER_METHOD(
        "MariaDB", "getTransactionStatistics", {}, [this](FunctionArguments& args) { return this->getTransactionStatistics(args); },
        Symbols::Variables::Type::OBJECT, "Get transaction usage statistics");
}

Symbols::ValuePtr MariaDBModule::connect(FunctionArguments& args) {
    try {
        validateConnectionParameters(args);
        
        if (args.size() != 5) {
            throw ConnectionException("connect expects (host, user, pass, db), got: " + 
                                    std::to_string(args.size() - 1) + " parameters");
        }

        // Extract object instance map
        if (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT) {
            throw ConnectionException("connect must be called on MariaDB instance");
        }

        Symbols::ObjectMap objMap = args[0];
        
        // Build connection configuration
        ConnectionConfig config;
        config.host = args[1]->get<std::string>();
        config.username = args[2]->get<std::string>();
        config.password = args[3]->get<std::string>();
        config.database = args[4]->get<std::string>();

        // Create and store connection
        auto connection = connection_manager_->createConnection(config);
        std::string conn_key = generateConnectionKey(config);
        
        {
            std::lock_guard<std::mutex> lock(module_mutex_);
            active_connections_[conn_key] = connection;
        }

        // Store connection information in object properties
        auto symbolContainer = Symbols::SymbolContainer::instance();
        symbolContainer->setObjectProperty("MariaDB", "__conn_key__", Symbols::ValuePtr(conn_key));
        symbolContainer->setObjectProperty("MariaDB", "__conn_id__", Symbols::ValuePtr(connection->getConnectionId()));
        symbolContainer->setObjectProperty("MariaDB", "__class__", Symbols::ValuePtr("MariaDB"));
        symbolContainer->setObjectProperty("MariaDB", "__host__", Symbols::ValuePtr(config.host));
        symbolContainer->setObjectProperty("MariaDB", "__database__", Symbols::ValuePtr(config.database));

        // Update object map for backward compatibility
        objMap["__conn_key__"] = symbolContainer->getObjectProperty("MariaDB", "__conn_key__");
        objMap["__conn_id__"] = symbolContainer->getObjectProperty("MariaDB", "__conn_id__");
        objMap["__class__"] = symbolContainer->getObjectProperty("MariaDB", "__class__");
        objMap["__host__"] = symbolContainer->getObjectProperty("MariaDB", "__host__");
        objMap["__database__"] = symbolContainer->getObjectProperty("MariaDB", "__database__");

        return Symbols::ValuePtr::makeClassInstance(objMap);
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "connect");
        throw; // Re-throw to maintain exception type
    } catch (const std::exception& e) {
        logError(e.what(), "connect");
        throw ConnectionException(std::string("Connection failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::disconnect(FunctionArguments& args) {
    try {
        auto connection = getConnectionFromArgs(args);
        if (connection) {
            connection->disconnect();
            
            // Remove from active connections
            auto symbolContainer = Symbols::SymbolContainer::instance();
            auto conn_key_prop = symbolContainer->getObjectProperty("MariaDB", "__conn_key__");
            if (conn_key_prop) {
                std::string conn_key = conn_key_prop;
                std::lock_guard<std::mutex> lock(module_mutex_);
                active_connections_.erase(conn_key);
            }
            
            // Clear object properties
            symbolContainer->setObjectProperty("MariaDB", "__conn_key__", nullptr);
            symbolContainer->setObjectProperty("MariaDB", "__conn_id__", nullptr);
        }
        
        return Symbols::ValuePtr::null();
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "disconnect");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "disconnect");
        throw ConnectionException(std::string("Disconnect failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::isConnected(FunctionArguments& args) {
    try {
        auto connection = getConnectionFromArgs(args);
        bool connected = connection && connection->isConnected();
        return Symbols::ValuePtr(connected);
        
    } catch (const std::exception& e) {
        logError(e.what(), "isConnected");
        return Symbols::ValuePtr(false);
    }
}

Symbols::ValuePtr MariaDBModule::reconnect(FunctionArguments& args) {
    try {
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw ConnectionException("No connection available to reconnect");
        }
        
        bool success = connection->reconnect();
        
        return Symbols::ValuePtr(success);
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "reconnect");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "reconnect");
        throw ConnectionException(std::string("Reconnect failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::query(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw QueryException("query expects (this, sql)");
        }

        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for query");
        }

        connection_manager_->validateConnection(connection);

        std::string sql = args[1];
        
        // Execute query with enhanced error handling
        MYSQL_RES* res = connection->executeQuery(sql);
        
        if (!res) {
            // Non-SELECT query (INSERT, UPDATE, DELETE, etc.)
            return Symbols::ValuePtr::null();
        }

        // Process SELECT query results
        unsigned int num_fields = mysql_num_fields(res);
        std::vector<std::string> fieldNames;
        
        // Fetch field names
        for (unsigned int i = 0; i < num_fields; ++i) {
            MYSQL_FIELD* field = mysql_fetch_field_direct(res, i);
            fieldNames.emplace_back(field->name);
        }

        // Collect rows into object map
        Symbols::ObjectMap result;
        MYSQL_ROW row;
        unsigned long* lengths;
        int rowIndex = 0;
        
        while ((row = mysql_fetch_row(res))) {
            lengths = mysql_fetch_lengths(res);
            Symbols::ObjectMap rowObj;
            
            for (unsigned int i = 0; i < num_fields; ++i) {
                std::string val = row[i] ? std::string(row[i], lengths[i]) : std::string();
                rowObj[fieldNames[i]] = Symbols::ValuePtr(val);
            }
            
            result[std::to_string(rowIndex++)] = Symbols::ValuePtr(rowObj);
        }
        
        mysql_free_result(res);
                
        return Symbols::ValuePtr(result);
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "query");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "query");
        throw QueryException(std::string("Query execution failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::close(FunctionArguments& args) {
    // Alias for disconnect
    return disconnect(args);
}

Symbols::ValuePtr MariaDBModule::escapeString(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw SecurityException("escapeString expects (this, input)");
        }

        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw SecurityException("No valid connection available for string escaping");
        }

        std::string input = args[1];
        std::string escaped = connection->escapeString(input);
        
        return Symbols::ValuePtr(escaped);
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "escapeString");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "escapeString");
        throw SecurityException(std::string("String escaping failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::getLastInsertId(FunctionArguments& args) {
    try {
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw ConnectionException("No valid connection available");
        }

        uint64_t last_id = connection->getLastInsertId();
        return Symbols::ValuePtr(static_cast<int>(last_id));
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "getLastInsertId");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "getLastInsertId");
        throw ConnectionException(std::string("Failed to get last insert ID: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::getAffectedRows(FunctionArguments& args) {
    try {
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw ConnectionException("No valid connection available");
        }

        uint64_t affected = connection->getAffectedRows();
        return Symbols::ValuePtr(static_cast<int>(affected));
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "getAffectedRows");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "getAffectedRows");
        throw ConnectionException(std::string("Failed to get affected rows: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::getConnectionInfo(FunctionArguments& args) {
    try {
        Symbols::ObjectMap info;
        
        // Try to get connection, but don't fail if it doesn't exist
        std::shared_ptr<DatabaseConnection> connection = nullptr;
        try {
            connection = getConnectionFromArgs(args);
        } catch (...) {
            // Connection doesn't exist yet, that's fine
        }
        
        if (connection) {
            info["connection_id"] = Symbols::ValuePtr(connection->getConnectionId());
            info["is_connected"] = Symbols::ValuePtr(connection->isConnected());
            info["is_healthy"] = Symbols::ValuePtr(connection->isHealthy());
            
            // Get additional info from symbol container
            auto symbolContainer = Symbols::SymbolContainer::instance();
            auto host_prop = symbolContainer->getObjectProperty("MariaDB", "__host__");
            auto db_prop = symbolContainer->getObjectProperty("MariaDB", "__database__");
            
            if (host_prop) {
                info["host"] = host_prop;
            } else {
                info["host"] = Symbols::ValuePtr("");
            }
            
            if (db_prop) {
                info["database"] = db_prop;
            } else {
                info["database"] = Symbols::ValuePtr("");
            }
        } else {
            // No connection established
            info["connection_id"] = Symbols::ValuePtr("");
            info["is_connected"] = Symbols::ValuePtr(false);
            info["is_healthy"] = Symbols::ValuePtr(false);
            info["host"] = Symbols::ValuePtr("");
            info["database"] = Symbols::ValuePtr("");
        }
        
        return Symbols::ValuePtr(info);
        
    } catch (const std::exception& e) {
        logError(e.what(), "getConnectionInfo");
        
        // Return default info on error instead of throwing
        Symbols::ObjectMap info;
        info["connection_id"] = Symbols::ValuePtr("");
        info["is_connected"] = Symbols::ValuePtr(false);
        info["is_healthy"] = Symbols::ValuePtr(false);
        info["host"] = Symbols::ValuePtr("");
        info["database"] = Symbols::ValuePtr("");
        info["error"] = Symbols::ValuePtr(e.what());
        
        return Symbols::ValuePtr(info);
    }
}

void MariaDBModule::initializeModule() {
        
    // Phase 1: Basic initialization
    // Future phases will add connection pooling, security framework, etc.
}

void MariaDBModule::cleanupConnections() {
    std::lock_guard<std::mutex> lock(module_mutex_);
    
    
    for (auto& [key, connection] : active_connections_) {
        if (connection && connection->isConnected()) {
            connection->disconnect();
        }
    }
    
    active_connections_.clear();
}

std::shared_ptr<DatabaseConnection> MariaDBModule::getConnectionFromArgs(const FunctionArguments& args) {
    if (args.empty()) {
        throw ConnectionException("Invalid arguments - no instance provided");
    }

    const auto& objVal = args[0];
    if (objVal != Symbols::Variables::Type::CLASS && objVal != Symbols::Variables::Type::OBJECT) {
        throw ConnectionException("Method must be called on MariaDB instance");
    }

    // Get connection key from symbol container
    auto symbolContainer = Symbols::SymbolContainer::instance();
    auto conn_key_prop = symbolContainer->getObjectProperty("MariaDB", "__conn_key__");
    
    if (!conn_key_prop) {
        return nullptr; // No connection established
    }

    std::string conn_key = conn_key_prop;
    
    std::lock_guard<std::mutex> lock(module_mutex_);
    auto it = active_connections_.find(conn_key);
    if (it != active_connections_.end()) {
        return it->second;
    }
    
    return nullptr;
}

void MariaDBModule::validateConnectionParameters(const FunctionArguments& args) {
    if (args.size() < 5) {
        throw ConnectionException("Insufficient connection parameters");
    }
    
    for (size_t i = 1; i <= 4; ++i) {
        if (args[i] != Symbols::Variables::Type::STRING) {
            throw ConnectionException("Connection parameter " + std::to_string(i) + " must be a string");
        }
        
        std::string param = args[i];
        if (param.empty() && i != 3) { // Password can be empty
            throw ConnectionException("Connection parameter " + std::to_string(i) + " cannot be empty");
        }
    }
}

std::string MariaDBModule::generateConnectionKey(const ConnectionConfig& config) {
    std::stringstream ss;
    ss << config.host << ":" << config.port << "/" << config.database 
       << "@" << config.username;
    return ss.str();
}

void MariaDBModule::handleDatabaseError(MYSQL* handle, const std::string& operation) {
    if (handle) {
        std::string error_msg = mysql_error(handle);
        int error_code = mysql_errno(handle);
        logError("MySQL Error " + std::to_string(error_code) + ": " + error_msg, operation);
    }
}

void MariaDBModule::logError(const std::string& error, const std::string& context) {
}

//=============================================================================
// Phase 2: Security Framework Method Implementations
//=============================================================================

Symbols::ValuePtr MariaDBModule::validateInput(FunctionArguments& args) {
    try {
        if (args.size() < 3) {
            throw SecurityException("validateInput expects (this, input, type)");
        }
        
        std::string input = args[1];
        std::string type = args[2];
        
        bool isValid = SecurityValidator::validateInput(input, type);
        
        return Symbols::ValuePtr(isValid);
        
    } catch (const SecurityException& e) {
        logSecurityEvent(e.what(), "validateInput");
        return Symbols::ValuePtr(false);
    } catch (const std::exception& e) {
        logError(e.what(), "validateInput");
        return Symbols::ValuePtr(false);
    }
}

Symbols::ValuePtr MariaDBModule::prepareStatement(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw SecurityException("prepareStatement expects (this, query)");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw SecurityException("No valid connection available for prepared statement");
        }
        
        std::string query = args[1];
        
        // Create unique key for this prepared statement
        std::string stmt_key = generateStatementKey(query);
        
        // Create and store prepared statement
        {
            std::lock_guard<std::mutex> lock(security_mutex_);
            prepared_statements_[stmt_key] = std::make_unique<PreparedStatement>(connection.get(), query);
        }
        
        
        return Symbols::ValuePtr(stmt_key);
        
    } catch (const SecurityException& e) {
        logSecurityEvent(e.what(), "prepareStatement");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "prepareStatement");
        throw SecurityException(std::string("Failed to prepare statement: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::bindParameter(FunctionArguments& args) {
    try {
        if (args.size() < 4) {
            throw SecurityException("bindParameter expects (this, stmt_key, index, value)");
        }
        
        std::string stmt_key = args[1];
        int index = args[2];
        Symbols::ValuePtr value = args[3];
        
        {
            std::lock_guard<std::mutex> lock(security_mutex_);
            auto it = prepared_statements_.find(stmt_key);
            if (it == prepared_statements_.end()) {
                throw SecurityException("Prepared statement not found: " + stmt_key);
            }
            
            bool success = it->second->bindParameter(index, value);
            return Symbols::ValuePtr(success);
        }
        
    } catch (const SecurityException& e) {
        logSecurityEvent(e.what(), "bindParameter");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "bindParameter");
        throw SecurityException(std::string("Failed to bind parameter: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::executeQuery(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw SecurityException("executeQuery expects (this, query, [parameters])");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw SecurityException("No valid connection available for query execution");
        }
        
        std::string query = args[1];
        
        // Validate query for security
        SecurityValidator::validateQuery(query);
        
        // If parameters provided, use prepared statement
        if (args.size() > 2 && args[2] != Symbols::Variables::Type::NULL_TYPE) {
            Symbols::ObjectMap params_obj = args[2];
            std::vector<Symbols::ValuePtr> parameters;
            
            // Convert object map to parameter vector
            for (const auto& [key, value] : params_obj) {
                parameters.push_back(value);
            }
            
            SecurityValidator::validateParameters(parameters);
            
            // Create temporary prepared statement
            PreparedStatement stmt(connection.get(), query);
            if (stmt.bindParameters(parameters)) {
                return stmt.executeQuery();
            } else {
                throw SecurityException("Failed to bind parameters for query execution");
            }
        } else {
            // Direct query execution (should be avoided for user input)
            MYSQL_RES* res = connection->executeQuery(query);
            
            if (!res) {
                return Symbols::ValuePtr::null();
            }
            
            // Process results similar to existing query method
            unsigned int num_fields = mysql_num_fields(res);
            std::vector<std::string> fieldNames;
            
            for (unsigned int i = 0; i < num_fields; ++i) {
                MYSQL_FIELD* field = mysql_fetch_field_direct(res, i);
                fieldNames.emplace_back(field->name);
            }
            
            Symbols::ObjectMap result;
            MYSQL_ROW row;
            unsigned long* lengths;
            int rowIndex = 0;
            
            while ((row = mysql_fetch_row(res))) {
                lengths = mysql_fetch_lengths(res);
                Symbols::ObjectMap rowObj;
                
                for (unsigned int i = 0; i < num_fields; ++i) {
                    std::string val = row[i] ? std::string(row[i], lengths[i]) : std::string();
                    rowObj[fieldNames[i]] = Symbols::ValuePtr(val);
                }
                
                result[std::to_string(rowIndex++)] = Symbols::ValuePtr(rowObj);
            }
            
            mysql_free_result(res);
            
            
            return Symbols::ValuePtr(result);
        }
        
    } catch (const SecurityException& e) {
        logSecurityEvent(e.what(), "executeQuery");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "executeQuery");
        throw SecurityException(std::string("Failed to execute query: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::executePrepared(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw SecurityException("executePrepared expects (this, stmt_key, [parameters])");
        }
        
        std::string stmt_key = args[1];
        
        {
            std::lock_guard<std::mutex> lock(security_mutex_);
            auto it = prepared_statements_.find(stmt_key);
            if (it == prepared_statements_.end()) {
                throw SecurityException("Prepared statement not found: " + stmt_key);
            }
            
            if (args.size() > 2 && args[2] != Symbols::Variables::Type::NULL_TYPE) {
                Symbols::ObjectMap params_obj = args[2];
                std::vector<Symbols::ValuePtr> parameters;
                
                // Convert object map to parameter vector
                for (const auto& [key, value] : params_obj) {
                    parameters.push_back(value);
                }
                
                if (!it->second->bindParameters(parameters)) {
                    throw SecurityException("Failed to bind parameters to prepared statement");
                }
            }
            
            return it->second->executeQuery();
        }
        
    } catch (const SecurityException& e) {
        logSecurityEvent(e.what(), "executePrepared");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "executePrepared");
        throw SecurityException(std::string("Failed to execute prepared statement: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::buildSelectQuery(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw SecurityException("buildSelectQuery expects (this, table, [columns], [conditions])");
        }
        
        std::string table = args[1];
        
        QueryBuilder builder;
        builder.from(table);
        
        // Handle columns parameter
        if (args.size() > 2 && args[2] != Symbols::Variables::Type::NULL_TYPE) {
            Symbols::ObjectMap columns_obj = args[2];
            std::vector<std::string> columns;
            
            for (const auto& [key, value] : columns_obj) {
                columns.push_back(value.get<std::string>());
            }
            
            builder.select(columns);
        } else {
            builder.select("*");
        }
        
        // Handle conditions parameter
        if (args.size() > 3 && args[3] != Symbols::Variables::Type::NULL_TYPE) {
            Symbols::ObjectMap conditions_obj = args[3];
            
            for (const auto& [column, value] : conditions_obj) {
                builder.whereEquals(column, value);
            }
        }
        
        std::string query = builder.buildQuery();
        
        return Symbols::ValuePtr(query);
        
    } catch (const SecurityException& e) {
        logSecurityEvent(e.what(), "buildSelectQuery");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "buildSelectQuery");
        throw SecurityException(std::string("Failed to build SELECT query: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::buildInsertQuery(FunctionArguments& args) {
    try {
        if (args.size() < 3) {
            throw SecurityException("buildInsertQuery expects (this, table, data)");
        }
        
        std::string table = args[1];
        Symbols::ObjectMap data_obj = args[2];
        
        QueryBuilder builder;
        builder.from(table);
        
        std::map<std::string, Symbols::ValuePtr> data;
        for (const auto& [column, value] : data_obj) {
            data[column] = value;
        }
        
        std::string query = builder.buildInsertQuery(data);
        
        return Symbols::ValuePtr(query);
        
    } catch (const SecurityException& e) {
        logSecurityEvent(e.what(), "buildInsertQuery");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "buildInsertQuery");
        throw SecurityException(std::string("Failed to build INSERT query: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::buildUpdateQuery(FunctionArguments& args) {
    try {
        if (args.size() < 4) {
            throw SecurityException("buildUpdateQuery expects (this, table, data, conditions)");
        }
        
        std::string table = args[1];
        Symbols::ObjectMap data_obj = args[2];
        Symbols::ObjectMap conditions_obj = args[3];
        
        QueryBuilder builder;
        builder.from(table);
        
        std::map<std::string, Symbols::ValuePtr> data;
        for (const auto& [column, value] : data_obj) {
            data[column] = value;
        }
        
        std::map<std::string, Symbols::ValuePtr> conditions;
        for (const auto& [column, value] : conditions_obj) {
            conditions[column] = value;
        }
        
        std::string query = builder.buildUpdateQuery(data, conditions);
        
        return Symbols::ValuePtr(query);
        
    } catch (const SecurityException& e) {
        logSecurityEvent(e.what(), "buildUpdateQuery");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "buildUpdateQuery");
        throw SecurityException(std::string("Failed to build UPDATE query: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::buildDeleteQuery(FunctionArguments& args) {
    try {
        if (args.size() < 3) {
            throw SecurityException("buildDeleteQuery expects (this, table, conditions)");
        }
        
        std::string table = args[1];
        Symbols::ObjectMap conditions_obj = args[2];
        
        QueryBuilder builder;
        builder.from(table);
        
        std::map<std::string, Symbols::ValuePtr> conditions;
        for (const auto& [column, value] : conditions_obj) {
            conditions[column] = value;
        }
        
        std::string query = builder.buildDeleteQuery(conditions);
        
        return Symbols::ValuePtr(query);
        
    } catch (const SecurityException& e) {
        logSecurityEvent(e.what(), "buildDeleteQuery");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "buildDeleteQuery");
        throw SecurityException(std::string("Failed to build DELETE query: ") + e.what());
    }
}

//=============================================================================
// Phase 2: Security Framework Helper Methods
//=============================================================================

void MariaDBModule::initializeSecurityFramework() {
    
    // Initialize security resources
    std::lock_guard<std::mutex> lock(security_mutex_);
    prepared_statements_.clear();
    query_builders_.clear();
}

void MariaDBModule::cleanupSecurityResources() {
    std::lock_guard<std::mutex> lock(security_mutex_);
    
    prepared_statements_.clear();
    
    query_builders_.clear();
}

std::string MariaDBModule::generateStatementKey(const std::string& query) {
    static std::atomic<int> statement_counter{1};
    
    std::stringstream ss;
    ss << "stmt_" << statement_counter.fetch_add(1) << "_";
    
    // Add hash of query for uniqueness
    std::hash<std::string> hasher;
    ss << std::hex << hasher(query);
    
    return ss.str();
}

void MariaDBModule::validateSecurityParameters(const FunctionArguments& args) {
    for (size_t i = 1; i < args.size(); ++i) {  // Skip 'this' parameter
        if (!isValidParameterType(args[i])) {
            throw SecurityException("Invalid parameter type at index " + std::to_string(i));
        }
    }
}

Symbols::ValuePtr MariaDBModule::sanitizeErrorMessage(const std::string& error) {
    // Remove sensitive information from error messages
    std::string sanitized = error;
    
    // Remove file paths
    std::regex path_pattern(R"([A-Za-z]:\\[^:\s]*|/[^:\s]*)");
    sanitized = std::regex_replace(sanitized, path_pattern, "[PATH]");
    
    // Remove IP addresses
    std::regex ip_pattern(R"(\b(?:[0-9]{1,3}\.){3}[0-9]{1,3}\b)");
    sanitized = std::regex_replace(sanitized, ip_pattern, "[IP]");
    
    // Remove specific error codes that might leak information
    std::regex error_code_pattern(R"(\b(errno|error)\s*=?\s*\d+\b)");
    sanitized = std::regex_replace(sanitized, error_code_pattern, "[ERROR_CODE]");
    
    return Symbols::ValuePtr(sanitized);
}

void MariaDBModule::logSecurityEvent(const std::string& event, const std::string& context) {
}

bool MariaDBModule::isValidParameterType(const Symbols::ValuePtr& param) {
    return SecurityValidator::isValidParameterType(param);
}

void MariaDBModule::validateParameterCount(const std::vector<Symbols::ValuePtr>& params, int expected) {
    if (static_cast<int>(params.size()) != expected) {
        throw SecurityException("Parameter count mismatch. Expected: " + std::to_string(expected) +
                               ", got: " + std::to_string(params.size()));
    }
}

std::string MariaDBModule::convertValueToString(const Symbols::ValuePtr& value) {
    if (!value) {
        return "";
    }
    
    switch (value.getType()) {
        case Symbols::Variables::Type::STRING:
            return value.get<std::string>();
        case Symbols::Variables::Type::INTEGER:
            return std::to_string(value.get<int>());
        case Symbols::Variables::Type::DOUBLE:
            return std::to_string(value.get<double>());
        case Symbols::Variables::Type::BOOLEAN:
            return value.get<bool>() ? "true" : "false";
        case Symbols::Variables::Type::NULL_TYPE:
            return "";
        default:
            throw SecurityException("Cannot convert parameter type to string");
    }
}

//=============================================================================
// Phase 3: Query Execution Engine Method Implementations
//=============================================================================

Symbols::ValuePtr MariaDBModule::select(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw QueryException("select expects (this, table, [columns], [conditions], [orderBy], [limit], [offset])");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for SELECT operation");
        }
        
        std::string table = args[1];
        
        // Extract optional parameters
        std::vector<std::string> columns = {"*"};
        if (args.size() > 2 && args[2] != Symbols::Variables::Type::NULL_TYPE) {
            columns = extractStringArrayFromArgs(args[2]);
        }
        
        std::map<std::string, Symbols::ValuePtr> conditions;
        if (args.size() > 3 && args[3] != Symbols::Variables::Type::NULL_TYPE) {
            conditions = extractObjectMapFromArgs(args[3]);
        }
        
        std::string orderBy;
        if (args.size() > 4 && args[4] != Symbols::Variables::Type::NULL_TYPE) {
            orderBy = args[4].get<std::string>();
        }
        
        int limit = -1;
        if (args.size() > 5 && args[5] != Symbols::Variables::Type::NULL_TYPE) {
            limit = args[5].get<int>();
        }
        
        int offset = 0;
        if (args.size() > 6 && args[6] != Symbols::Variables::Type::NULL_TYPE) {
            offset = args[6].get<int>();
        }
        
        auto resultSet = query_executor_->select(table, columns, conditions, orderBy, limit, offset, connection.get());
        
        if (resultSet) {
            // Convert ResultSet to VoidScript object format
            Symbols::ObjectMap result;
            int rowIndex = 0;
            
            while (resultSet->next()) {
                Symbols::ObjectMap rowData;
                for (unsigned int i = 0; i < resultSet->getColumnCount(); ++i) {
                    std::string columnName = resultSet->getColumnName(i);
                    std::string value = resultSet->getString(i);
                    rowData[columnName] = Symbols::ValuePtr(value);
                }
                result[std::to_string(rowIndex++)] = Symbols::ValuePtr(rowData);
            }
            
            return Symbols::ValuePtr(result);
        }
        
        return Symbols::ValuePtr::null();
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "select");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "select");
        throw QueryException(std::string("SELECT operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::selectOne(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw QueryException("selectOne expects (this, table, [columns], [conditions])");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for selectOne operation");
        }
        
        std::string table = args[1];
        
        std::vector<std::string> columns = {"*"};
        if (args.size() > 2 && args[2] != Symbols::Variables::Type::NULL_TYPE) {
            columns = extractStringArrayFromArgs(args[2]);
        }
        
        std::map<std::string, Symbols::ValuePtr> conditions;
        if (args.size() > 3 && args[3] != Symbols::Variables::Type::NULL_TYPE) {
            conditions = extractObjectMapFromArgs(args[3]);
        }
        
        return query_executor_->selectOne(table, columns, conditions, connection.get());
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "selectOne");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "selectOne");
        throw QueryException(std::string("selectOne operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::selectColumn(FunctionArguments& args) {
    try {
        if (args.size() < 3) {
            throw QueryException("selectColumn expects (this, table, column, [conditions])");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for selectColumn operation");
        }
        
        std::string table = args[1];
        std::string column = args[2];
        
        std::map<std::string, Symbols::ValuePtr> conditions;
        if (args.size() > 3 && args[3] != Symbols::Variables::Type::NULL_TYPE) {
            conditions = extractObjectMapFromArgs(args[3]);
        }
        
        std::string value = query_executor_->selectColumn(table, column, conditions, 0, connection.get());
        return Symbols::ValuePtr(value);
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "selectColumn");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "selectColumn");
        throw QueryException(std::string("selectColumn operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::selectScalar(FunctionArguments& args) {
    try {
        if (args.size() < 3) {
            throw QueryException("selectScalar expects (this, table, column, [conditions])");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for selectScalar operation");
        }
        
        std::string table = args[1];
        std::string column = args[2];
        
        std::map<std::string, Symbols::ValuePtr> conditions;
        if (args.size() > 3 && args[3] != Symbols::Variables::Type::NULL_TYPE) {
            conditions = extractObjectMapFromArgs(args[3]);
        }
        
        return query_executor_->selectScalar(table, column, conditions, connection.get());
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "selectScalar");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "selectScalar");
        throw QueryException(std::string("selectScalar operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::insert(FunctionArguments& args) {
    try {
        if (args.size() < 3) {
            throw QueryException("insert expects (this, table, data)");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for INSERT operation");
        }
        
        std::string table = args[1];
        std::map<std::string, Symbols::ValuePtr> data = extractObjectMapFromArgs(args[2]);
        
        uint64_t insertId = query_executor_->insert(table, data, connection.get());
        
        return Symbols::ValuePtr(static_cast<int>(insertId));
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "insert");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "insert");
        throw QueryException(std::string("INSERT operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::insertBatch(FunctionArguments& args) {
    try {
        if (args.size() < 3) {
            throw QueryException("insertBatch expects (this, table, dataArray)");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for batch INSERT operation");
        }
        
        std::string table = args[1];
        std::vector<std::map<std::string, Symbols::ValuePtr>> dataArray = extractDataArrayFromArgs(args[2]);
        
        std::vector<uint64_t> insertIds = query_executor_->insertBatch(table, dataArray, connection.get());
        
        // Convert result to VoidScript object
        Symbols::ObjectMap result;
        for (size_t i = 0; i < insertIds.size(); ++i) {
            result[std::to_string(i)] = Symbols::ValuePtr(static_cast<int>(insertIds[i]));
        }
        
        return Symbols::ValuePtr(result);
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "insertBatch");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "insertBatch");
        throw QueryException(std::string("Batch INSERT operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::insertAndGetId(FunctionArguments& args) {
    try {
        if (args.size() < 3) {
            throw QueryException("insertAndGetId expects (this, table, data)");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for insertAndGetId operation");
        }
        
        std::string table = args[1];
        std::map<std::string, Symbols::ValuePtr> data = extractObjectMapFromArgs(args[2]);
        
        uint64_t insertId = query_executor_->insertAndGetId(table, data, connection.get());
        
        return Symbols::ValuePtr(static_cast<int>(insertId));
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "insertAndGetId");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "insertAndGetId");
        throw QueryException(std::string("insertAndGetId operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::update(FunctionArguments& args) {
    try {
        if (args.size() < 4) {
            throw QueryException("update expects (this, table, data, conditions)");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for UPDATE operation");
        }
        
        std::string table = args[1];
        std::map<std::string, Symbols::ValuePtr> data = extractObjectMapFromArgs(args[2]);
        std::map<std::string, Symbols::ValuePtr> conditions = extractObjectMapFromArgs(args[3]);
        
        int affectedRows = query_executor_->update(table, data, conditions, connection.get());
        
        return Symbols::ValuePtr(affectedRows);
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "update");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "update");
        throw QueryException(std::string("UPDATE operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::updateBatch(FunctionArguments& args) {
    try {
        if (args.size() < 4) {
            throw QueryException("updateBatch expects (this, table, dataArray, keyColumn)");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for batch UPDATE operation");
        }
        
        std::string table = args[1];
        std::vector<std::map<std::string, Symbols::ValuePtr>> dataArray = extractDataArrayFromArgs(args[2]);
        std::string keyColumn = args[3].get<std::string>();
        
        std::vector<int> results = query_executor_->updateBatch(table, dataArray, keyColumn, connection.get());
        
        // Convert result to VoidScript object
        Symbols::ObjectMap result;
        for (size_t i = 0; i < results.size(); ++i) {
            result[std::to_string(i)] = Symbols::ValuePtr(results[i]);
        }
        
        return Symbols::ValuePtr(result);
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "updateBatch");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "updateBatch");
        throw QueryException(std::string("Batch UPDATE operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::deleteRecord(FunctionArguments& args) {
    try {
        if (args.size() < 3) {
            throw QueryException("deleteRecord expects (this, table, conditions)");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for DELETE operation");
        }
        
        std::string table = args[1];
        std::map<std::string, Symbols::ValuePtr> conditions = extractObjectMapFromArgs(args[2]);
        
        int affectedRows = query_executor_->deleteRecord(table, conditions, connection.get());
        
        return Symbols::ValuePtr(affectedRows);
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "deleteRecord");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "deleteRecord");
        throw QueryException(std::string("DELETE operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::deleteBatch(FunctionArguments& args) {
    try {
        if (args.size() < 4) {
            throw QueryException("deleteBatch expects (this, table, keyValues, keyColumn)");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for batch DELETE operation");
        }
        
        std::string table = args[1];
        std::vector<Symbols::ValuePtr> keyValues = extractValueArrayFromArgs(args[2]);
        std::string keyColumn = args[3].get<std::string>();
        
        std::vector<int> results = query_executor_->deleteBatch(table, keyValues, keyColumn, connection.get());
        
        // Convert result to VoidScript object
        Symbols::ObjectMap result;
        for (size_t i = 0; i < results.size(); ++i) {
            result[std::to_string(i)] = Symbols::ValuePtr(results[i]);
        }
        
        return Symbols::ValuePtr(result);
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "deleteBatch");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "deleteBatch");
        throw QueryException(std::string("Batch DELETE operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::createTable(FunctionArguments& args) {
    try {
        if (args.size() < 3) {
            throw QueryException("createTable expects (this, tableName, columns, [constraints])");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for CREATE TABLE operation");
        }
        
        std::string tableName = args[1];
        
        // Extract columns from object map
        Symbols::ObjectMap columnsObj = args[2];
        std::map<std::string, std::string> columns;
        for (const auto& [key, value] : columnsObj) {
            columns[key] = value.get<std::string>();
        }
        
        // Extract constraints if provided
        std::vector<std::string> constraints;
        if (args.size() > 3 && args[3] != Symbols::Variables::Type::NULL_TYPE) {
            constraints = extractStringArrayFromArgs(args[3]);
        }
        
        bool success = query_executor_->createTable(tableName, columns, constraints, connection.get());
        
        return Symbols::ValuePtr(success);
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "createTable");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "createTable");
        throw QueryException(std::string("CREATE TABLE operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::dropTable(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw QueryException("dropTable expects (this, tableName, [ifExists])");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for DROP TABLE operation");
        }
        
        std::string tableName = args[1];
        
        bool ifExists = true;
        if (args.size() > 2 && args[2] != Symbols::Variables::Type::NULL_TYPE) {
            ifExists = args[2].get<bool>();
        }
        
        bool success = query_executor_->dropTable(tableName, ifExists, connection.get());
        
        return Symbols::ValuePtr(success);
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "dropTable");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "dropTable");
        throw QueryException(std::string("DROP TABLE operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::createIndex(FunctionArguments& args) {
    try {
        if (args.size() < 3) {
            throw QueryException("createIndex expects (this, tableName, columns, [indexName], [unique])");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for CREATE INDEX operation");
        }
        
        std::string tableName = args[1];
        std::vector<std::string> columns = extractStringArrayFromArgs(args[2]);
        
        std::string indexName;
        if (args.size() > 3 && args[3] != Symbols::Variables::Type::NULL_TYPE) {
            indexName = args[3].get<std::string>();
        }
        
        bool unique = false;
        if (args.size() > 4 && args[4] != Symbols::Variables::Type::NULL_TYPE) {
            unique = args[4].get<bool>();
        }
        
        bool success = query_executor_->createIndex(tableName, columns, indexName, unique, connection.get());
        
        return Symbols::ValuePtr(success);
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "createIndex");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "createIndex");
        throw QueryException(std::string("CREATE INDEX operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::dropIndex(FunctionArguments& args) {
    try {
        if (args.size() < 3) {
            throw QueryException("dropIndex expects (this, tableName, indexName, [ifExists])");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for DROP INDEX operation");
        }
        
        std::string tableName = args[1];
        std::string indexName = args[2];
        
        bool ifExists = true;
        if (args.size() > 3 && args[3] != Symbols::Variables::Type::NULL_TYPE) {
            ifExists = args[3].get<bool>();
        }
        
        bool success = query_executor_->dropIndex(tableName, indexName, ifExists, connection.get());
        
        return Symbols::ValuePtr(success);
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "dropIndex");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "dropIndex");
        throw QueryException(std::string("DROP INDEX operation failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::getRowCount(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw QueryException("getRowCount expects (this, table, [conditions])");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw QueryException("No valid connection available for row count operation");
        }
        
        std::string table = args[1];
        
        std::map<std::string, Symbols::ValuePtr> conditions;
        if (args.size() > 2 && args[2] != Symbols::Variables::Type::NULL_TYPE) {
            conditions = extractObjectMapFromArgs(args[2]);
        }
        
        uint64_t rowCount = query_executor_->getRowCount(table, conditions, connection.get());
        
        return Symbols::ValuePtr(static_cast<int>(rowCount));
        
    } catch (const DatabaseException& e) {
        logError(e.what(), "getRowCount");
        throw;
    } catch (const std::exception& e) {
        logError(e.what(), "getRowCount");
        throw QueryException(std::string("Row count operation failed: ") + e.what());
    }
}

//=============================================================================
// Phase 3: Helper Method Implementations
//=============================================================================

void MariaDBModule::initializeQueryExecutor() {
    
    // Query executor is already initialized in constructor
    // Additional initialization if needed can be added here
}

std::map<std::string, Symbols::ValuePtr> MariaDBModule::extractObjectMapFromArgs(const Symbols::ValuePtr& arg) {
    std::map<std::string, Symbols::ValuePtr> result;
    
    if (arg == Symbols::Variables::Type::OBJECT) {
        Symbols::ObjectMap objMap = arg;
        for (const auto& [key, value] : objMap) {
            result[key] = value;
        }
    }
    
    return result;
}

std::vector<std::string> MariaDBModule::extractStringArrayFromArgs(const Symbols::ValuePtr& arg) {
    std::vector<std::string> result;
    
    if (arg == Symbols::Variables::Type::OBJECT) {
        Symbols::ObjectMap objMap = arg;
        for (const auto& [key, value] : objMap) {
            result.push_back(value.get<std::string>());
        }
    }
    
    return result;
}

std::vector<std::map<std::string, Symbols::ValuePtr>> MariaDBModule::extractDataArrayFromArgs(const Symbols::ValuePtr& arg) {
    std::vector<std::map<std::string, Symbols::ValuePtr>> result;
    
    if (arg == Symbols::Variables::Type::OBJECT) {
        Symbols::ObjectMap arrayMap = arg;
        for (const auto& [key, value] : arrayMap) {
            if (value == Symbols::Variables::Type::OBJECT) {
                std::map<std::string, Symbols::ValuePtr> dataMap;
                Symbols::ObjectMap objMap = value;
                for (const auto& [dataKey, dataValue] : objMap) {
                    dataMap[dataKey] = dataValue;
                }
                result.push_back(dataMap);
            }
        }
    }
    
    return result;
}

std::vector<Symbols::ValuePtr> MariaDBModule::extractValueArrayFromArgs(const Symbols::ValuePtr& arg) {
    std::vector<Symbols::ValuePtr> result;
    
    if (arg == Symbols::Variables::Type::OBJECT) {
        Symbols::ObjectMap objMap = arg;
        for (const auto& [key, value] : objMap) {
            result.push_back(value);
        }
    }
    
    return result;
}

//=============================================================================
// Phase 3: ResultSet Implementation
//=============================================================================

ResultSet::ResultSet(MYSQL_RES* result)
    : result_(result)
    , current_row_(nullptr)
    , row_lengths_(nullptr)
    , num_fields_(0)
    , num_rows_(0)
    , current_row_index_(0)
    , has_current_row_(false)
    , owns_result_(true)
{
    if (result_) {
        initializeMetadata();
    }
}

ResultSet::~ResultSet() {
    cleanup();
}

ResultSet::ResultSet(ResultSet&& other) noexcept
    : result_(other.result_)
    , current_row_(other.current_row_)
    , row_lengths_(other.row_lengths_)
    , column_names_(std::move(other.column_names_))
    , column_types_(std::move(other.column_types_))
    , num_fields_(other.num_fields_)
    , num_rows_(other.num_rows_)
    , current_row_index_(other.current_row_index_)
    , has_current_row_(other.has_current_row_)
    , owns_result_(other.owns_result_)
{
    other.result_ = nullptr;
    other.current_row_ = nullptr;
    other.row_lengths_ = nullptr;
    other.owns_result_ = false;
    other.has_current_row_ = false;
}

ResultSet& ResultSet::operator=(ResultSet&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        result_ = other.result_;
        current_row_ = other.current_row_;
        row_lengths_ = other.row_lengths_;
        column_names_ = std::move(other.column_names_);
        column_types_ = std::move(other.column_types_);
        num_fields_ = other.num_fields_;
        num_rows_ = other.num_rows_;
        current_row_index_ = other.current_row_index_;
        has_current_row_ = other.has_current_row_;
        owns_result_ = other.owns_result_;
        
        other.result_ = nullptr;
        other.current_row_ = nullptr;
        other.row_lengths_ = nullptr;
        other.owns_result_ = false;
        other.has_current_row_ = false;
    }
    return *this;
}

bool ResultSet::next() {
    if (!result_) {
        return false;
    }
    
    current_row_ = mysql_fetch_row(result_);
    if (current_row_) {
        row_lengths_ = mysql_fetch_lengths(result_);
        has_current_row_ = true;
        current_row_index_++;
        return true;
    } else {
        has_current_row_ = false;
        row_lengths_ = nullptr;
        return false;
    }
}

bool ResultSet::hasNext() const {
    if (!result_ || !has_current_row_) {
        return false;
    }
    return current_row_index_ < num_rows_;
}

void ResultSet::reset() {
    if (result_) {
        mysql_data_seek(result_, 0);
        current_row_index_ = 0;
        has_current_row_ = false;
        current_row_ = nullptr;
        row_lengths_ = nullptr;
    }
}

bool ResultSet::first() {
    reset();
    return next();
}

bool ResultSet::last() {
    if (!result_ || num_rows_ == 0) {
        return false;
    }
    
    mysql_data_seek(result_, num_rows_ - 1);
    current_row_index_ = num_rows_ - 1;
    return next();
}

std::string ResultSet::getString(int columnIndex) const {
    validateColumnIndex(columnIndex);
    
    if (!has_current_row_ || !current_row_) {
        throw QueryException("No current row available");
    }
    
    return getRawValue(columnIndex);
}

int ResultSet::getInt(int columnIndex) const {
    std::string value = getString(columnIndex);
    if (value.empty()) {
        return 0;
    }
    
    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        throw QueryException("Cannot convert value to integer: " + value);
    }
}

double ResultSet::getDouble(int columnIndex) const {
    std::string value = getString(columnIndex);
    if (value.empty()) {
        return 0.0;
    }
    
    try {
        return std::stod(value);
    } catch (const std::exception&) {
        throw QueryException("Cannot convert value to double: " + value);
    }
}

bool ResultSet::getBool(int columnIndex) const {
    std::string value = getString(columnIndex);
    if (value.empty() || value == "0" || value == "false") {
        return false;
    }
    return true;
}

bool ResultSet::isNull(int columnIndex) const {
    validateColumnIndex(columnIndex);
    
    if (!has_current_row_ || !current_row_) {
        throw QueryException("No current row available");
    }
    
    return current_row_[columnIndex] == nullptr;
}

std::string ResultSet::getString(const std::string& columnName) const {
    return getString(getColumnIndex(columnName));
}

int ResultSet::getInt(const std::string& columnName) const {
    return getInt(getColumnIndex(columnName));
}

double ResultSet::getDouble(const std::string& columnName) const {
    return getDouble(getColumnIndex(columnName));
}

bool ResultSet::getBool(const std::string& columnName) const {
    return getBool(getColumnIndex(columnName));
}

bool ResultSet::isNull(const std::string& columnName) const {
    return isNull(getColumnIndex(columnName));
}

std::string ResultSet::getColumnName(int index) const {
    validateColumnIndex(index);
    return column_names_[index];
}

enum_field_types ResultSet::getColumnType(int index) const {
    validateColumnIndex(index);
    return column_types_[index];
}

Symbols::ValuePtr ResultSet::toVoidScriptObject() const {
    if (!result_) {
        return Symbols::ValuePtr::null();
    }
    
    Symbols::ObjectMap result;
    
    // Add metadata
    Symbols::ObjectMap metadata;
    metadata["column_count"] = Symbols::ValuePtr(static_cast<int>(num_fields_));
    metadata["row_count"] = Symbols::ValuePtr(static_cast<int>(num_rows_));
    metadata["current_row"] = Symbols::ValuePtr(static_cast<int>(current_row_index_));
    
    Symbols::ObjectMap columns;
    for (size_t i = 0; i < column_names_.size(); ++i) {
        columns[std::to_string(i)] = Symbols::ValuePtr(column_names_[i]);
    }
    metadata["columns"] = Symbols::ValuePtr(columns);
    
    result["metadata"] = Symbols::ValuePtr(metadata);
    
    // Add current row data if available
    if (has_current_row_ && current_row_) {
        Symbols::ObjectMap row_data;
        for (unsigned int i = 0; i < num_fields_; ++i) {
            std::string value = getRawValue(i);
            row_data[column_names_[i]] = Symbols::ValuePtr(value);
        }
        result["current_row"] = Symbols::ValuePtr(row_data);
    } else {
        result["current_row"] = Symbols::ValuePtr::null();
    }
    
    return Symbols::ValuePtr(result);
}

void ResultSet::initializeMetadata() {
    if (!result_) {
        return;
    }
    
    num_fields_ = mysql_num_fields(result_);
    num_rows_ = mysql_num_rows(result_);
    
    column_names_.reserve(num_fields_);
    column_types_.reserve(num_fields_);
    
    for (unsigned int i = 0; i < num_fields_; ++i) {
        MYSQL_FIELD* field = mysql_fetch_field_direct(result_, i);
        if (field) {
            column_names_.emplace_back(field->name);
            column_types_.emplace_back(field->type);
        }
    }
}

int ResultSet::getColumnIndex(const std::string& columnName) const {
    for (size_t i = 0; i < column_names_.size(); ++i) {
        if (column_names_[i] == columnName) {
            return static_cast<int>(i);
        }
    }
    throw QueryException("Column not found: " + columnName);
}

void ResultSet::validateColumnIndex(int index) const {
    if (index < 0 || index >= static_cast<int>(num_fields_)) {
        throw QueryException("Column index out of range: " + std::to_string(index) +
                           " (valid range: 0-" + std::to_string(num_fields_ - 1) + ")");
    }
}

std::string ResultSet::getRawValue(int columnIndex) const {
    if (!has_current_row_ || !current_row_ || !row_lengths_) {
        return "";
    }
    
    if (current_row_[columnIndex] == nullptr) {
        return "";
    }
    
    return std::string(current_row_[columnIndex], row_lengths_[columnIndex]);
}

void ResultSet::cleanup() {
    if (result_ && owns_result_) {
        mysql_free_result(result_);
    }
    result_ = nullptr;
    current_row_ = nullptr;
    row_lengths_ = nullptr;
    has_current_row_ = false;
    owns_result_ = false;
}

//=============================================================================
// Phase 3: BatchProcessor Implementation
//=============================================================================

BatchProcessor::BatchProcessor(DatabaseConnection* connection, const std::string& operation_type)
    : connection_(connection)
    , operation_type_(operation_type)
    , batch_size_limit_(1000)
    , use_transactions_(true)
{
    if (!connection_) {
        throw QueryException("Invalid connection provided to BatchProcessor");
    }
    
    // Validate operation type
    if (operation_type_ != "INSERT" && operation_type_ != "UPDATE" && operation_type_ != "DELETE") {
        throw QueryException("Invalid operation type for BatchProcessor: " + operation_type_);
    }
}

void BatchProcessor::addBatchData(const std::map<std::string, Symbols::ValuePtr>& data) {
    if (batch_data_.size() >= batch_size_limit_) {
        throw QueryException("Batch size limit exceeded: " + std::to_string(batch_size_limit_));
    }
    
    // Validate data
    for (const auto& [key, value] : data) {
        SecurityValidator::validateColumnName(key);
        if (!SecurityValidator::isValidParameterType(value)) {
            throw SecurityException("Invalid parameter type in batch data for column: " + key);
        }
    }
    
    batch_data_.push_back(data);
}

void BatchProcessor::addBatchData(const std::vector<std::map<std::string, Symbols::ValuePtr>>& data_list) {
    for (const auto& data : data_list) {
        addBatchData(data);
    }
}

void BatchProcessor::clearBatch() {
    batch_data_.clear();
}

std::vector<int> BatchProcessor::executeBatch() {
    if (isEmpty()) {
        return {};
    }
    
    validateBatchData();
    validateTableName();
    
    std::vector<int> results;
    
    if (operation_type_ == "INSERT") {
        int result = executeInsertBatch();
        results.push_back(result);
    } else {
        throw QueryException("Unsupported batch operation for this method: " + operation_type_);
    }
    
    return results;
}

int BatchProcessor::executeInsertBatch() {
    if (isEmpty()) {
        return 0;
    }
    
    std::string query = buildBatchInsertQuery();
    std::vector<Symbols::ValuePtr> parameters = flattenBatchParameters();
    
    bool transaction_started = false;
    
    try {
        // Start transaction if enabled
        if (use_transactions_) {
            if (connection_->executeNonQuery("START TRANSACTION")) {
                transaction_started = true;
            }
        }
        
        // Execute the batch insert
        PreparedStatement stmt(connection_, query);
        if (!stmt.bindParameters(parameters)) {
            throw QueryException("Failed to bind parameters for batch insert");
        }
        
        int affected_rows = stmt.executeUpdate();
        
        // Commit transaction
        if (transaction_started) {
            connection_->executeNonQuery("COMMIT");
        }
        
        
        clearBatch();
        return affected_rows;
        
    } catch (const std::exception& e) {
        // Rollback transaction on error
        if (transaction_started) {
            connection_->executeNonQuery("ROLLBACK");
        }
        throw QueryException("Batch insert failed: " + std::string(e.what()));
    }
}

int BatchProcessor::executeUpdateBatch(const std::map<std::string, Symbols::ValuePtr>& conditions) {
    if (isEmpty()) {
        return 0;
    }
    
    int total_affected = 0;
    bool transaction_started = false;
    
    try {
        // Start transaction if enabled
        if (use_transactions_) {
            if (connection_->executeNonQuery("START TRANSACTION")) {
                transaction_started = true;
            }
        }
        
        // Execute individual updates
        for (const auto& data : batch_data_) {
            QueryBuilder builder;
            std::string query = builder.from(table_name_).buildUpdateQuery(data, conditions);
            std::vector<Symbols::ValuePtr> parameters;
            
            // Add data parameters
            for (const auto& [key, value] : data) {
                parameters.push_back(value);
            }
            
            // Add condition parameters
            for (const auto& [key, value] : conditions) {
                parameters.push_back(value);
            }
            
            PreparedStatement stmt(connection_, query);
            if (stmt.bindParameters(parameters)) {
                total_affected += stmt.executeUpdate();
            }
        }
        
        // Commit transaction
        if (transaction_started) {
            connection_->executeNonQuery("COMMIT");
        }
        
        
        clearBatch();
        return total_affected;
        
    } catch (const std::exception& e) {
        // Rollback transaction on error
        if (transaction_started) {
            connection_->executeNonQuery("ROLLBACK");
        }
        throw QueryException("Batch update failed: " + std::string(e.what()));
    }
}

int BatchProcessor::executeDeleteBatch(const std::map<std::string, Symbols::ValuePtr>& conditions) {
    if (isEmpty()) {
        return 0;
    }
    
    int total_affected = 0;
    bool transaction_started = false;
    
    try {
        // Start transaction if enabled
        if (use_transactions_) {
            if (connection_->executeNonQuery("START TRANSACTION")) {
                transaction_started = true;
            }
        }
        
        // Execute individual deletes
        for (const auto& data : batch_data_) {
            // Combine data with conditions for WHERE clause
            std::map<std::string, Symbols::ValuePtr> delete_conditions = conditions;
            for (const auto& [key, value] : data) {
                delete_conditions[key] = value;
            }
            
            QueryBuilder builder;
            std::string query = builder.from(table_name_).buildDeleteQuery(delete_conditions);
            std::vector<Symbols::ValuePtr> parameters;
            
            // Add condition parameters
            for (const auto& [key, value] : delete_conditions) {
                parameters.push_back(value);
            }
            
            PreparedStatement stmt(connection_, query);
            if (stmt.bindParameters(parameters)) {
                total_affected += stmt.executeUpdate();
            }
        }
        
        // Commit transaction
        if (transaction_started) {
            connection_->executeNonQuery("COMMIT");
        }
        
        
        clearBatch();
        return total_affected;
        
    } catch (const std::exception& e) {
        // Rollback transaction on error
        if (transaction_started) {
            connection_->executeNonQuery("ROLLBACK");
        }
        throw QueryException("Batch delete failed: " + std::string(e.what()));
    }
}

void BatchProcessor::validateBatchData() {
    if (isEmpty()) {
        throw QueryException("Batch is empty");
    }
    
    // Validate that all rows have the same columns
    if (!batch_data_.empty()) {
        const auto& first_row = batch_data_[0];
        for (size_t i = 1; i < batch_data_.size(); ++i) {
            if (batch_data_[i].size() != first_row.size()) {
                throw QueryException("Inconsistent column count in batch data at row " + std::to_string(i));
            }
            
            for (const auto& [key, value] : first_row) {
                if (batch_data_[i].find(key) == batch_data_[i].end()) {
                    throw QueryException("Missing column '" + key + "' in batch data at row " + std::to_string(i));
                }
            }
        }
    }
}

void BatchProcessor::validateTableName() {
    if (table_name_.empty()) {
        throw QueryException("Table name not set for batch operation");
    }
    
    SecurityValidator::validateTableName(table_name_);
}

std::string BatchProcessor::buildBatchInsertQuery() {
    if (isEmpty()) {
        throw QueryException("Cannot build query for empty batch");
    }
    
    const auto& first_row = batch_data_[0];
    std::stringstream query;
    
    query << "INSERT INTO `" << table_name_ << "` (";
    
    bool first_column = true;
    for (const auto& [column, value] : first_row) {
        if (!first_column) {
            query << ", ";
        }
        query << "`" << column << "`";
        first_column = false;
    }
    
    query << ") VALUES ";
    
    for (size_t i = 0; i < batch_data_.size(); ++i) {
        if (i > 0) {
            query << ", ";
        }
        query << "(";
        
        bool first_param = true;
        for (const auto& [column, value] : first_row) {
            if (!first_param) {
                query << ", ";
            }
            query << "?";
            first_param = false;
        }
        
        query << ")";
    }
    
    return query.str();
}

std::string BatchProcessor::buildBatchUpdateQuery(const std::map<std::string, Symbols::ValuePtr>& conditions) {
    // For batch updates, we'll use individual queries
    // This is a placeholder - actual implementation would be in executeUpdateBatch
    return "";
}

std::string BatchProcessor::buildBatchDeleteQuery(const std::map<std::string, Symbols::ValuePtr>& conditions) {
    // For batch deletes, we'll use individual queries
    // This is a placeholder - actual implementation would be in executeDeleteBatch
    return "";
}

std::vector<Symbols::ValuePtr> BatchProcessor::flattenBatchParameters() {
    std::vector<Symbols::ValuePtr> parameters;
    
    if (isEmpty()) {
        return parameters;
    }
    
    const auto& first_row = batch_data_[0];
    
    for (const auto& row : batch_data_) {
        for (const auto& [column, value] : first_row) {
            auto it = row.find(column);
            if (it != row.end()) {
                parameters.push_back(it->second);
            } else {
                parameters.push_back(Symbols::ValuePtr::null());
            }
        }
    }
    
    return parameters;
}

//=============================================================================
// Phase 3: QueryExecutor Implementation
//=============================================================================

QueryExecutor::QueryExecutor(ConnectionManager* connection_manager)
    : connection_manager_(connection_manager)
{
    if (!connection_manager_) {
        throw QueryException("Invalid connection manager provided to QueryExecutor");
    }
}

QueryExecutor::~QueryExecutor() {
    clearStatementCache();
}

std::unique_ptr<ResultSet> QueryExecutor::executeQuery(const std::string& query,
                                                      const std::vector<Symbols::ValuePtr>& parameters,
                                                      DatabaseConnection* connection) {
    SecurityValidator::validateQuery(query);
    SecurityValidator::validateParameters(parameters);
    
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw QueryException("No valid connection available for query execution");
    }
    
    if (parameters.empty()) {
        // Direct query execution
        MYSQL_RES* result = conn->executeQuery(query);
        return std::make_unique<ResultSet>(result);
    } else {
        // Prepared statement execution
        PreparedStatement stmt(conn, query);
        if (!stmt.bindParameters(parameters)) {
            throw QueryException("Failed to bind parameters for query execution");
        }
        
        // For now, we'll use the existing executeQuery method
        // Full implementation would return a proper ResultSet
        stmt.executeQuery();
        
        // This is a simplified implementation
        // In a full implementation, we'd need to modify PreparedStatement to return MYSQL_RES*
        throw QueryException("Parameterized queries with ResultSet not fully implemented yet");
    }
}

bool QueryExecutor::executeNonQuery(const std::string& query,
                                   const std::vector<Symbols::ValuePtr>& parameters,
                                   DatabaseConnection* connection) {
    SecurityValidator::validateQuery(query);
    SecurityValidator::validateParameters(parameters);
    
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw QueryException("No valid connection available for non-query execution");
    }
    
    if (parameters.empty()) {
        return conn->executeNonQuery(query);
    } else {
        PreparedStatement stmt(conn, query);
        if (!stmt.bindParameters(parameters)) {
            throw QueryException("Failed to bind parameters for non-query execution");
        }
        
        return stmt.execute();
    }
}

std::unique_ptr<ResultSet> QueryExecutor::select(const std::string& table,
                                                const std::vector<std::string>& columns,
                                                const std::map<std::string, Symbols::ValuePtr>& conditions,
                                                const std::string& orderBy,
                                                int limit, int offset,
                                                DatabaseConnection* connection) {
    validateTableName(table);
    validateColumnNames(columns);
    
    std::string query = buildSelectQuery(table, columns, conditions, orderBy, limit, offset);
    std::vector<Symbols::ValuePtr> parameters = extractParameters(conditions);
    
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw QueryException("No valid connection available for SELECT operation");
    }
    
    if (parameters.empty()) {
        MYSQL_RES* result = conn->executeQuery(query);
        return std::make_unique<ResultSet>(result);
    } else {
        PreparedStatement stmt(conn, query);
        if (!stmt.bindParameters(parameters)) {
            throw QueryException("Failed to bind parameters for SELECT operation");
        }
        
        // This is a simplified implementation
        stmt.executeQuery();
        throw QueryException("Parameterized SELECT with ResultSet not fully implemented yet");
    }
}

Symbols::ValuePtr QueryExecutor::selectOne(const std::string& table,
                                          const std::vector<std::string>& columns,
                                          const std::map<std::string, Symbols::ValuePtr>& conditions,
                                          DatabaseConnection* connection) {
    auto resultSet = select(table, columns, conditions, "", 1, 0, connection);
    
    if (resultSet && resultSet->next()) {
        return resultSet->toVoidScriptObject();
    }
    
    return Symbols::ValuePtr::null();
}

std::string QueryExecutor::selectColumn(const std::string& table, const std::string& column,
                                       const std::map<std::string, Symbols::ValuePtr>& conditions,
                                       int columnIndex,
                                       DatabaseConnection* connection) {
    auto resultSet = select(table, {column}, conditions, "", 1, 0, connection);
    
    if (resultSet && resultSet->next()) {
        return resultSet->getString(columnIndex);
    }
    
    return "";
}

Symbols::ValuePtr QueryExecutor::selectScalar(const std::string& table, const std::string& column,
                                             const std::map<std::string, Symbols::ValuePtr>& conditions,
                                             DatabaseConnection* connection) {
    std::string value = selectColumn(table, column, conditions, 0, connection);
    return Symbols::ValuePtr(value);
}

uint64_t QueryExecutor::insert(const std::string& table,
                              const std::map<std::string, Symbols::ValuePtr>& data,
                              DatabaseConnection* connection) {
    validateTableName(table);
    
    if (data.empty()) {
        throw QueryException("No data provided for INSERT operation");
    }
    
    std::string query = buildInsertQuery(table, data);
    std::vector<Symbols::ValuePtr> parameters = extractParameters(data);
    
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw QueryException("No valid connection available for INSERT operation");
    }
    
    PreparedStatement stmt(conn, query);
    if (!stmt.bindParameters(parameters)) {
        throw QueryException("Failed to bind parameters for INSERT operation");
    }
    
    if (stmt.execute()) {
        return stmt.getLastInsertId();
    }
    
    throw QueryException("INSERT operation failed");
}

std::vector<uint64_t> QueryExecutor::insertBatch(const std::string& table,
                                                 const std::vector<std::map<std::string, Symbols::ValuePtr>>& dataArray,
                                                 DatabaseConnection* connection) {
    validateTableName(table);
    
    if (dataArray.empty()) {
        throw QueryException("No data provided for batch INSERT operation");
    }
    
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw QueryException("No valid connection available for batch INSERT operation");
    }
    
    BatchProcessor processor(conn, "INSERT");
    processor.setTableName(table);
    processor.addBatchData(dataArray);
    
    std::vector<int> results = processor.executeBatch();
    
    // Convert to uint64_t vector
    std::vector<uint64_t> insert_ids;
    if (!results.empty()) {
        // For batch inserts, we can only return the last insert ID
        // Individual IDs would require separate INSERT statements
        insert_ids.push_back(conn->getLastInsertId());
    }
    
    return insert_ids;
}

uint64_t QueryExecutor::insertAndGetId(const std::string& table,
                                      const std::map<std::string, Symbols::ValuePtr>& data,
                                      DatabaseConnection* connection) {
    return insert(table, data, connection);
}

int QueryExecutor::update(const std::string& table,
                         const std::map<std::string, Symbols::ValuePtr>& data,
                         const std::map<std::string, Symbols::ValuePtr>& conditions,
                         DatabaseConnection* connection) {
    validateTableName(table);
    
    if (data.empty()) {
        throw QueryException("No data provided for UPDATE operation");
    }
    
    if (conditions.empty()) {
        throw QueryException("No conditions provided for UPDATE operation - this would update all rows");
    }
    
    std::string query = buildUpdateQuery(table, data, conditions);
    std::vector<Symbols::ValuePtr> data_params = extractParameters(data);
    std::vector<Symbols::ValuePtr> condition_params = extractParameters(conditions);
    std::vector<Symbols::ValuePtr> parameters = combineParameters(data_params, condition_params);
    
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw QueryException("No valid connection available for UPDATE operation");
    }
    
    PreparedStatement stmt(conn, query);
    if (!stmt.bindParameters(parameters)) {
        throw QueryException("Failed to bind parameters for UPDATE operation");
    }
    
    return stmt.executeUpdate();
}

std::vector<int> QueryExecutor::updateBatch(const std::string& table,
                                           const std::vector<std::map<std::string, Symbols::ValuePtr>>& dataArray,
                                           const std::string& keyColumn,
                                           DatabaseConnection* connection) {
    validateTableName(table);
    SecurityValidator::validateColumnName(keyColumn);
    
    if (dataArray.empty()) {
        throw QueryException("No data provided for batch UPDATE operation");
    }
    
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw QueryException("No valid connection available for batch UPDATE operation");
    }
    
    std::vector<int> results;
    bool transaction_started = false;
    
    try {
        // Start transaction for batch operation
        if (conn->executeNonQuery("START TRANSACTION")) {
            transaction_started = true;
        }
        
        // Process updates efficiently with proper error handling
        for (size_t i = 0; i < dataArray.size(); ++i) {
            const auto& data = dataArray[i];
            auto it = data.find(keyColumn);
            if (it == data.end()) {
                throw QueryException("Key column '" + keyColumn + "' not found in update data at index " + std::to_string(i));
            }
            
            std::map<std::string, Symbols::ValuePtr> update_data = data;
            update_data.erase(keyColumn);  // Remove key from update data
            
            if (update_data.empty()) {
                results.push_back(0); // No columns to update
                continue;
            }
            
            std::map<std::string, Symbols::ValuePtr> conditions;
            conditions[keyColumn] = it->second;
            
            // Execute individual update with proper error handling
            try {
                int affected = update(table, update_data, conditions, conn);
                results.push_back(affected);
            } catch (const std::exception& e) {
                if (transaction_started) {
                    conn->executeNonQuery("ROLLBACK");
                }
                throw QueryException("Batch update failed at index " + std::to_string(i) + ": " + e.what());
            }
        }
        
        // Commit transaction
        if (transaction_started) {
            conn->executeNonQuery("COMMIT");
        }
        
    } catch (const std::exception& e) {
        if (transaction_started) {
            conn->executeNonQuery("ROLLBACK");
        }
        throw;
    }
    
    return results;
}

int QueryExecutor::deleteRecord(const std::string& table,
                               const std::map<std::string, Symbols::ValuePtr>& conditions,
                               DatabaseConnection* connection) {
    validateTableName(table);
    
    if (conditions.empty()) {
        throw QueryException("No conditions provided for DELETE operation - this would delete all rows");
    }
    
    std::string query = buildDeleteQuery(table, conditions);
    std::vector<Symbols::ValuePtr> parameters = extractParameters(conditions);
    
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw QueryException("No valid connection available for DELETE operation");
    }
    
    PreparedStatement stmt(conn, query);
    if (!stmt.bindParameters(parameters)) {
        throw QueryException("Failed to bind parameters for DELETE operation");
    }
    
    return stmt.executeUpdate();
}

std::vector<int> QueryExecutor::deleteBatch(const std::string& table,
                                           const std::vector<Symbols::ValuePtr>& keyValues,
                                           const std::string& keyColumn,
                                           DatabaseConnection* connection) {
    validateTableName(table);
    SecurityValidator::validateColumnName(keyColumn);
    
    if (keyValues.empty()) {
        throw QueryException("No key values provided for batch DELETE operation");
    }
    
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw QueryException("No valid connection available for batch DELETE operation");
    }
    
    std::vector<int> results;
    bool transaction_started = false;
    
    try {
        // Start transaction for batch operation
        if (conn->executeNonQuery("START TRANSACTION")) {
            transaction_started = true;
        }
        
        // Process deletes efficiently with proper error handling
        for (size_t i = 0; i < keyValues.size(); ++i) {
            const auto& keyValue = keyValues[i];
            
            // Validate key value
            if (!SecurityValidator::isValidParameterType(keyValue)) {
                throw QueryException("Invalid key value type at index " + std::to_string(i));
            }
            
            std::map<std::string, Symbols::ValuePtr> conditions;
            conditions[keyColumn] = keyValue;
            
            // Execute individual delete with proper error handling
            try {
                int affected = deleteRecord(table, conditions, conn);
                results.push_back(affected);
            } catch (const std::exception& e) {
                if (transaction_started) {
                    conn->executeNonQuery("ROLLBACK");
                }
                throw QueryException("Batch delete failed at index " + std::to_string(i) + ": " + e.what());
            }
        }
        
        // Commit transaction
        if (transaction_started) {
            conn->executeNonQuery("COMMIT");
        }
        
    } catch (const std::exception& e) {
        if (transaction_started) {
            conn->executeNonQuery("ROLLBACK");
        }
        throw;
    }
    
    return results;
}

bool QueryExecutor::createTable(const std::string& tableName,
                               const std::map<std::string, std::string>& columns,
                               const std::vector<std::string>& constraints,
                               DatabaseConnection* connection) {
    validateTableName(tableName);
    
    if (columns.empty()) {
        throw QueryException("No columns provided for CREATE TABLE operation");
    }
    
    std::stringstream query;
    query << "CREATE TABLE `" << tableName << "` (";
    
    bool first = true;
    for (const auto& [columnName, columnType] : columns) {
        SecurityValidator::validateColumnName(columnName);
        
        if (!first) {
            query << ", ";
        }
        query << "`" << columnName << "` " << columnType;
        first = false;
    }
    
    // Add constraints
    for (const auto& constraint : constraints) {
        query << ", " << constraint;
    }
    
    query << ")";
    
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw QueryException("No valid connection available for CREATE TABLE operation");
    }
    
    return conn->executeNonQuery(query.str());
}

bool QueryExecutor::dropTable(const std::string& tableName,
                             bool ifExists,
                             DatabaseConnection* connection) {
    validateTableName(tableName);
    
    std::string query = "DROP TABLE ";
    if (ifExists) {
        query += "IF EXISTS ";
    }
    query += "`" + tableName + "`";
    
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw QueryException("No valid connection available for DROP TABLE operation");
    }
    
    return conn->executeNonQuery(query);
}

bool QueryExecutor::createIndex(const std::string& tableName,
                               const std::vector<std::string>& columns,
                               const std::string& indexName,
                               bool unique,
                               DatabaseConnection* connection) {
    validateTableName(tableName);
    validateColumnNames(columns);
    
    if (columns.empty()) {
        throw QueryException("No columns provided for CREATE INDEX operation");
    }
    
    std::string actualIndexName = indexName.empty() ?
        ("idx_" + tableName + "_" + columns[0]) : indexName;
    
    SecurityValidator::validateIdentifier(actualIndexName);
    
    std::stringstream query;
    query << "CREATE ";
    if (unique) {
        query << "UNIQUE ";
    }
    query << "INDEX `" << actualIndexName << "` ON `" << tableName << "` (";
    
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) {
            query << ", ";
        }
        query << "`" << columns[i] << "`";
    }
    
    query << ")";
    
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw QueryException("No valid connection available for CREATE INDEX operation");
    }
    
    return conn->executeNonQuery(query.str());
}

bool QueryExecutor::dropIndex(const std::string& tableName,
                             const std::string& indexName,
                             bool ifExists,
                             DatabaseConnection* connection) {
    validateTableName(tableName);
    SecurityValidator::validateIdentifier(indexName);
    
    std::string query = "DROP INDEX ";
    if (ifExists) {
        query += "IF EXISTS ";
    }
    query += "`" + indexName + "` ON `" + tableName + "`";
    
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw QueryException("No valid connection available for DROP INDEX operation");
    }
    
    return conn->executeNonQuery(query);
}

uint64_t QueryExecutor::getLastInsertId(DatabaseConnection* connection) {
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw ConnectionException("No valid connection available");
    }
    
    return conn->getLastInsertId();
}

uint64_t QueryExecutor::getAffectedRows(DatabaseConnection* connection) {
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw ConnectionException("No valid connection available");
    }
    
    return conn->getAffectedRows();
}

uint64_t QueryExecutor::getRowCount(const std::string& table,
                                   const std::map<std::string, Symbols::ValuePtr>& conditions,
                                   DatabaseConnection* connection) {
    validateTableName(table);
    
    std::string query = "SELECT COUNT(*) FROM `" + table + "`";
    std::vector<Symbols::ValuePtr> parameters;
    
    if (!conditions.empty()) {
        query += " WHERE ";
        bool first = true;
        for (const auto& [column, value] : conditions) {
            SecurityValidator::validateColumnName(column);
            if (!first) {
                query += " AND ";
            }
            query += "`" + column + "` = ?";
            parameters.push_back(value);
            first = false;
        }
    }
    
    DatabaseConnection* conn = getConnection(connection);
    if (!conn) {
        throw QueryException("No valid connection available for row count operation");
    }
    
    PreparedStatement stmt(conn, query);
    if (!parameters.empty() && !stmt.bindParameters(parameters)) {
        throw QueryException("Failed to bind parameters for row count operation");
    }
    
    // Execute the count query using direct query execution for better reliability
    if (parameters.empty()) {
        // Use direct query for simple count
        MYSQL_RES* result = conn->executeQuery(query);
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row && row[0]) {
                uint64_t count = std::strtoull(row[0], nullptr, 10);
                mysql_free_result(result);
                return count;
            }
            mysql_free_result(result);
        }
    } else {
        // For parameterized queries, use prepared statement approach
        if (stmt.execute()) {
            // Get the result using the existing getResult method
            Symbols::ValuePtr result_obj = stmt.getResult();
            if (result_obj && result_obj.getType() == Symbols::Variables::Type::OBJECT) {
                Symbols::ObjectMap obj_map = result_obj;
                // Look for the count column (typically named "COUNT(*)")
                for (const auto& [key, value] : obj_map) {
                    if (value && value.getType() == Symbols::Variables::Type::STRING) {
                        std::string count_str = value.get<std::string>();
                        return std::strtoull(count_str.c_str(), nullptr, 10);
                    }
                }
            }
        }
    }
    
    return 0;
}

void QueryExecutor::clearStatementCache() {
    std::lock_guard<std::mutex> lock(executor_mutex_);
    cached_statements_.clear();
}

size_t QueryExecutor::getCacheSize() const {
    std::lock_guard<std::mutex> lock(executor_mutex_);
    return cached_statements_.size();
}

std::string QueryExecutor::buildSelectQuery(const std::string& table,
                                           const std::vector<std::string>& columns,
                                           const std::map<std::string, Symbols::ValuePtr>& conditions,
                                           const std::string& orderBy,
                                           int limit, int offset) {
    std::stringstream query;
    
    query << "SELECT ";
    
    if (columns.empty() || (columns.size() == 1 && columns[0] == "*")) {
        query << "*";
    } else {
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0) {
                query << ", ";
            }
            query << "`" << columns[i] << "`";
        }
    }
    
    query << " FROM `" << table << "`";
    
    if (!conditions.empty()) {
        query << " WHERE ";
        bool first = true;
        for (const auto& [column, value] : conditions) {
            if (!first) {
                query << " AND ";
            }
            query << "`" << column << "` = ?";
            first = false;
        }
    }
    
    if (!orderBy.empty()) {
        query << " ORDER BY " << orderBy;
    }
    
    if (limit > 0) {
        query << " LIMIT " << limit;
        if (offset > 0) {
            query << " OFFSET " << offset;
        }
    }
    
    return query.str();
}

std::string QueryExecutor::buildInsertQuery(const std::string& table,
                                           const std::map<std::string, Symbols::ValuePtr>& data) {
    std::stringstream query;
    
    query << "INSERT INTO `" << table << "` (";
    
    bool first = true;
    for (const auto& [column, value] : data) {
        if (!first) {
            query << ", ";
        }
        query << "`" << column << "`";
        first = false;
    }
    
    query << ") VALUES (";
    
    first = true;
    for (const auto& [column, value] : data) {
        if (!first) {
            query << ", ";
        }
        query << "?";
        first = false;
    }
    
    query << ")";
    
    return query.str();
}

std::string QueryExecutor::buildUpdateQuery(const std::string& table,
                                           const std::map<std::string, Symbols::ValuePtr>& data,
                                           const std::map<std::string, Symbols::ValuePtr>& conditions) {
    std::stringstream query;
    
    query << "UPDATE `" << table << "` SET ";
    
    bool first = true;
    for (const auto& [column, value] : data) {
        if (!first) {
            query << ", ";
        }
        query << "`" << column << "` = ?";
        first = false;
    }
    
    query << " WHERE ";
    
    first = true;
    for (const auto& [column, value] : conditions) {
        if (!first) {
            query << " AND ";
        }
        query << "`" << column << "` = ?";
        first = false;
    }
    
    return query.str();
}

std::string QueryExecutor::buildDeleteQuery(const std::string& table,
                                           const std::map<std::string, Symbols::ValuePtr>& conditions) {
    std::stringstream query;
    
    query << "DELETE FROM `" << table << "`";
    
    if (!conditions.empty()) {
        query << " WHERE ";
        bool first = true;
        for (const auto& [column, value] : conditions) {
            if (!first) {
                query << " AND ";
            }
            query << "`" << column << "` = ?";
            first = false;
        }
    }
    
    return query.str();
}

std::vector<Symbols::ValuePtr> QueryExecutor::extractParameters(const std::map<std::string, Symbols::ValuePtr>& data) {
    std::vector<Symbols::ValuePtr> parameters;
    for (const auto& [column, value] : data) {
        parameters.push_back(value);
    }
    return parameters;
}

std::vector<Symbols::ValuePtr> QueryExecutor::combineParameters(const std::vector<Symbols::ValuePtr>& params1,
                                                               const std::vector<Symbols::ValuePtr>& params2) {
    std::vector<Symbols::ValuePtr> combined = params1;
    combined.insert(combined.end(), params2.begin(), params2.end());
    return combined;
}

DatabaseConnection* QueryExecutor::getConnection(DatabaseConnection* provided_connection) {
    if (provided_connection) {
        return provided_connection;
    }
    
    // For now, we'll return null - this would need to be integrated with the module's connection management
    return nullptr;
}

std::string QueryExecutor::generateStatementKey(const std::string& query) {
    std::hash<std::string> hasher;
    return "stmt_" + std::to_string(hasher(query));
}

void QueryExecutor::validateTableName(const std::string& tableName) {
    if (!SecurityValidator::validateTableName(tableName)) {
        throw SecurityException("Invalid table name: " + tableName);
    }
}

void QueryExecutor::validateColumnNames(const std::vector<std::string>& columns) {
    for (const auto& column : columns) {
        if (column != "*" && !SecurityValidator::validateColumnName(column)) {
            throw SecurityException("Invalid column name: " + column);
        }
    }
}

//=============================================================================
// Phase 4: MariaDBModule Transaction Method Implementations
//=============================================================================

Symbols::ValuePtr MariaDBModule::beginTransaction(FunctionArguments& args) {
    try {
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw TransactionException("No valid connection available for transaction begin");
        }
        
        if (!transaction_manager_) {
            transaction_manager_ = std::make_unique<TransactionManager>(connection.get());
        }
        
        bool success = transaction_manager_->beginTransaction();
        
        return Symbols::ValuePtr(success);
        
    } catch (const TransactionException& e) {
        logTransactionEvent(e.what(), "beginTransaction");
        throw;
    } catch (const std::exception& e) {
        logTransactionEvent(e.what(), "beginTransaction");
        throw TransactionException(std::string("Begin transaction failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::commitTransaction(FunctionArguments& args) {
    try {
        if (!transaction_manager_) {
            throw TransactionException("No transaction manager available");
        }
        
        bool success = transaction_manager_->commitTransaction();
        
        return Symbols::ValuePtr(success);
        
    } catch (const TransactionException& e) {
        logTransactionEvent(e.what(), "commitTransaction");
        throw;
    } catch (const std::exception& e) {
        logTransactionEvent(e.what(), "commitTransaction");
        throw TransactionException(std::string("Commit transaction failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::rollbackTransaction(FunctionArguments& args) {
    try {
        if (!transaction_manager_) {
            throw TransactionException("No transaction manager available");
        }
        
        bool success = transaction_manager_->rollbackTransaction();
        
        return Symbols::ValuePtr(success);
        
    } catch (const TransactionException& e) {
        logTransactionEvent(e.what(), "rollbackTransaction");
        throw;
    } catch (const std::exception& e) {
        logTransactionEvent(e.what(), "rollbackTransaction");
        throw TransactionException(std::string("Rollback transaction failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::isInTransaction(FunctionArguments& args) {
    try {
        if (!transaction_manager_) {
            return Symbols::ValuePtr(false);
        }
        
        bool inTransaction = transaction_manager_->isInTransaction();
        return Symbols::ValuePtr(inTransaction);
        
    } catch (const std::exception& e) {
        logTransactionEvent(e.what(), "isInTransaction");
        return Symbols::ValuePtr(false);
    }
}

Symbols::ValuePtr MariaDBModule::createSavepoint(FunctionArguments& args) {
    try {
        if (!transaction_manager_) {
            throw TransactionException("No transaction manager available");
        }
        
        std::string name;
        if (args.size() > 1 && args[1] != Symbols::Variables::Type::NULL_TYPE) {
            name = args[1].get<std::string>();
        }
        
        std::string savepoint_name = transaction_manager_->createSavepoint(name);
        
        return Symbols::ValuePtr(savepoint_name);
        
    } catch (const TransactionException& e) {
        logTransactionEvent(e.what(), "createSavepoint");
        throw;
    } catch (const std::exception& e) {
        logTransactionEvent(e.what(), "createSavepoint");
        throw TransactionException(std::string("Create savepoint failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::rollbackToSavepoint(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw TransactionException("rollbackToSavepoint expects (this, name)");
        }
        
        if (!transaction_manager_) {
            throw TransactionException("No transaction manager available");
        }
        
        std::string name = args[1].get<std::string>();
        bool success = transaction_manager_->rollbackToSavepoint(name);
        
        return Symbols::ValuePtr(success);
        
    } catch (const TransactionException& e) {
        logTransactionEvent(e.what(), "rollbackToSavepoint");
        throw;
    } catch (const std::exception& e) {
        logTransactionEvent(e.what(), "rollbackToSavepoint");
        throw TransactionException(std::string("Rollback to savepoint failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::releaseSavepoint(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw TransactionException("releaseSavepoint expects (this, name)");
        }
        
        if (!transaction_manager_) {
            throw TransactionException("No transaction manager available");
        }
        
        std::string name = args[1].get<std::string>();
        bool success = transaction_manager_->releaseSavepoint(name);
        
        return Symbols::ValuePtr(success);
        
    } catch (const TransactionException& e) {
        logTransactionEvent(e.what(), "releaseSavepoint");
        throw;
    } catch (const std::exception& e) {
        logTransactionEvent(e.what(), "releaseSavepoint");
        throw TransactionException(std::string("Release savepoint failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::withTransaction(FunctionArguments& args) {
    try {
        // This would be implemented with callback support in a more advanced version
        // For now, just begin a transaction and return the transaction manager state
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw TransactionException("No valid connection available");
        }
        
        if (!transaction_manager_) {
            transaction_manager_ = std::make_unique<TransactionManager>(connection.get());
        }
        
        bool success = transaction_manager_->beginTransaction();
        return Symbols::ValuePtr(success);
        
    } catch (const TransactionException& e) {
        logTransactionEvent(e.what(), "withTransaction");
        throw;
    } catch (const std::exception& e) {
        logTransactionEvent(e.what(), "withTransaction");
        throw TransactionException(std::string("WithTransaction failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::withSavepoint(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw TransactionException("withSavepoint expects (this, name)");
        }
        
        if (!transaction_manager_) {
            throw TransactionException("No transaction manager available");
        }
        
        std::string name = args[1].get<std::string>();
        std::string savepoint_name = transaction_manager_->createSavepoint(name);
        
        return Symbols::ValuePtr(savepoint_name);
        
    } catch (const TransactionException& e) {
        logTransactionEvent(e.what(), "withSavepoint");
        throw;
    } catch (const std::exception& e) {
        logTransactionEvent(e.what(), "withSavepoint");
        throw TransactionException(std::string("WithSavepoint failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::setIsolationLevel(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw TransactionException("setIsolationLevel expects (this, level)");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw TransactionException("No valid connection available");
        }
        
        if (!transaction_manager_) {
            transaction_manager_ = std::make_unique<TransactionManager>(connection.get());
        }
        
        std::string level_str = args[1].get<std::string>();
        
        TransactionManager::IsolationLevel level;
        if (level_str == "READ_UNCOMMITTED") {
            level = TransactionManager::IsolationLevel::READ_UNCOMMITTED;
        } else if (level_str == "READ_COMMITTED") {
            level = TransactionManager::IsolationLevel::READ_COMMITTED;
        } else if (level_str == "REPEATABLE_READ") {
            level = TransactionManager::IsolationLevel::REPEATABLE_READ;
        } else if (level_str == "SERIALIZABLE") {
            level = TransactionManager::IsolationLevel::SERIALIZABLE;
        } else {
            throw TransactionException("Invalid isolation level: " + level_str);
        }
        
        bool success = transaction_manager_->setIsolationLevel(level);
        
        return Symbols::ValuePtr(success);
        
    } catch (const TransactionException& e) {
        logTransactionEvent(e.what(), "setIsolationLevel");
        throw;
    } catch (const std::exception& e) {
        logTransactionEvent(e.what(), "setIsolationLevel");
        throw TransactionException(std::string("Set isolation level failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::getIsolationLevel(FunctionArguments& args) {
    try {
        if (!transaction_manager_) {
            return Symbols::ValuePtr("REPEATABLE READ"); // Default
        }
        
        std::string level = transaction_manager_->getIsolationLevelString();
        return Symbols::ValuePtr(level);
        
    } catch (const std::exception& e) {
        logTransactionEvent(e.what(), "getIsolationLevel");
        return Symbols::ValuePtr("REPEATABLE READ"); // Default on error
    }
}

Symbols::ValuePtr MariaDBModule::setAutoCommit(FunctionArguments& args) {
    try {
        if (args.size() < 2) {
            throw TransactionException("setAutoCommit expects (this, enabled)");
        }
        
        auto connection = getConnectionFromArgs(args);
        if (!connection) {
            throw TransactionException("No valid connection available");
        }
        
        if (!transaction_manager_) {
            transaction_manager_ = std::make_unique<TransactionManager>(connection.get());
        }
        
        bool enabled = args[1].get<bool>();
        bool success = transaction_manager_->setAutoCommit(enabled);
        
        return Symbols::ValuePtr(success);
        
    } catch (const TransactionException& e) {
        logTransactionEvent(e.what(), "setAutoCommit");
        throw;
    } catch (const std::exception& e) {
        logTransactionEvent(e.what(), "setAutoCommit");
        throw TransactionException(std::string("Set auto-commit failed: ") + e.what());
    }
}

Symbols::ValuePtr MariaDBModule::getAutoCommit(FunctionArguments& args) {
    try {
        if (!transaction_manager_) {
            return Symbols::ValuePtr(true); // Default auto-commit is enabled
        }
        
        bool autoCommit = transaction_manager_->getAutoCommit();
        return Symbols::ValuePtr(autoCommit);
        
    } catch (const std::exception& e) {
        logTransactionEvent(e.what(), "getAutoCommit");
        return Symbols::ValuePtr(true); // Default on error
    }
}

Symbols::ValuePtr MariaDBModule::detectDeadlock(FunctionArguments& args) {
    try {
        if (!transaction_manager_) {
            return Symbols::ValuePtr(false);
        }
        
        bool deadlockDetected = transaction_manager_->detectDeadlock();
        return Symbols::ValuePtr(deadlockDetected);
        
    } catch (const std::exception& e) {
        logTransactionEvent(e.what(), "detectDeadlock");
        return Symbols::ValuePtr(false);
    }
}

Symbols::ValuePtr MariaDBModule::getTransactionStatistics(FunctionArguments& args) {
    try {
        Symbols::ObjectMap stats;
        
        if (transaction_manager_) {
            stats["transaction_count"] = Symbols::ValuePtr(transaction_manager_->getTransactionCount());
            stats["rollback_count"] = Symbols::ValuePtr(transaction_manager_->getRollbackCount());
            stats["deadlock_count"] = Symbols::ValuePtr(transaction_manager_->getDeadlockCount());
            stats["savepoint_count"] = Symbols::ValuePtr(static_cast<int>(transaction_manager_->getSavepointCount()));
            stats["is_in_transaction"] = Symbols::ValuePtr(transaction_manager_->isInTransaction());
            stats["auto_commit_enabled"] = Symbols::ValuePtr(transaction_manager_->getAutoCommit());
            stats["isolation_level"] = Symbols::ValuePtr(transaction_manager_->getIsolationLevelString());
        } else {
            stats["transaction_count"] = Symbols::ValuePtr(0);
            stats["rollback_count"] = Symbols::ValuePtr(0);
            stats["deadlock_count"] = Symbols::ValuePtr(0);
            stats["savepoint_count"] = Symbols::ValuePtr(0);
            stats["is_in_transaction"] = Symbols::ValuePtr(false);
            stats["auto_commit_enabled"] = Symbols::ValuePtr(true);
            stats["isolation_level"] = Symbols::ValuePtr("REPEATABLE READ");
        }
        
        return Symbols::ValuePtr(stats);
        
    } catch (const std::exception& e) {
        logTransactionEvent(e.what(), "getTransactionStatistics");
        
        // Return default stats on error
        Symbols::ObjectMap stats;
        stats["transaction_count"] = Symbols::ValuePtr(0);
        stats["rollback_count"] = Symbols::ValuePtr(0);
        stats["deadlock_count"] = Symbols::ValuePtr(0);
        stats["savepoint_count"] = Symbols::ValuePtr(0);
        stats["is_in_transaction"] = Symbols::ValuePtr(false);
        stats["auto_commit_enabled"] = Symbols::ValuePtr(true);
        stats["isolation_level"] = Symbols::ValuePtr("REPEATABLE READ");
        stats["error"] = Symbols::ValuePtr(e.what());
        
        return Symbols::ValuePtr(stats);
    }
}

//=============================================================================
// Phase 4: Transaction Management Helper Methods
//=============================================================================

void MariaDBModule::initializeTransactionManager() {
    
    // Transaction manager will be created on-demand when first needed
    // Additional initialization if needed can be added here
}

void MariaDBModule::cleanupTransactionResources() {
    std::lock_guard<std::mutex> lock(transaction_mutex_);
    
    
    // Cleanup transaction scopes
    for (auto& [key, scope] : active_transaction_scopes_) {
        if (scope && scope->isActive() && !scope->isCommitted()) {
            try {
                scope->rollback();
            } catch (...) {
                // Ignore errors during cleanup
            }
        }
    }
    active_transaction_scopes_.clear();
    
    // Cleanup transaction manager
    if (transaction_manager_) {
        if (transaction_manager_->isInTransaction()) {
            try {
                transaction_manager_->rollbackTransaction();
            } catch (...) {
                // Ignore errors during cleanup
            }
        }
        transaction_manager_.reset();
    }
}

TransactionManager* MariaDBModule::getTransactionManager(const FunctionArguments& args) {
    auto connection = getConnectionFromArgs(args);
    if (!connection) {
        throw TransactionException("No valid connection available for transaction manager");
    }
    
    if (!transaction_manager_) {
        transaction_manager_ = std::make_unique<TransactionManager>(connection.get());
    }
    
    return transaction_manager_.get();
}

std::string MariaDBModule::generateTransactionScopeKey() {
    static std::atomic<int> scope_counter{1};
    
    std::stringstream ss;
    ss << "txn_scope_" << scope_counter.fetch_add(1) << "_";
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    ss << std::put_time(std::localtime(&time_t), "%H%M%S");
    
    return ss.str();
}

void MariaDBModule::validateTransactionState(const FunctionArguments& args) {
    auto connection = getConnectionFromArgs(args);
    if (!connection) {
        throw TransactionException("No valid connection available for transaction operations");
    }
    
    if (!connection->isConnected()) {
        throw TransactionException("Connection is not active for transaction operations");
    }
}

bool MariaDBModule::executeInTransaction(std::function<bool()> operation, bool use_savepoint) {
    if (!transaction_manager_) {
        throw TransactionException("No transaction manager available");
    }
    
    try {
        if (use_savepoint && transaction_manager_->isInTransaction()) {
            // Execute within a savepoint
            std::string savepoint_name = transaction_manager_->createSavepoint();
            
            try {
                bool success = operation();
                if (success) {
                    transaction_manager_->releaseSavepoint(savepoint_name);
                } else {
                    transaction_manager_->rollbackToSavepoint(savepoint_name);
                }
                return success;
            } catch (const std::exception& e) {
                transaction_manager_->rollbackToSavepoint(savepoint_name);
                throw;
            }
        } else {
            // Execute within a full transaction
            bool transaction_started = false;
            if (!transaction_manager_->isInTransaction()) {
                transaction_manager_->beginTransaction();
                transaction_started = true;
            }
            
            try {
                bool success = operation();
                if (transaction_started) {
                    if (success) {
                        transaction_manager_->commitTransaction();
                    } else {
                        transaction_manager_->rollbackTransaction();
                    }
                }
                return success;
            } catch (const std::exception& e) {
                if (transaction_started) {
                    transaction_manager_->rollbackTransaction();
                }
                throw;
            }
        }
    } catch (const std::exception& e) {
        handleTransactionError(e, "executeInTransaction");
        return false;
    }
}

void MariaDBModule::handleTransactionError(const std::exception& e, const std::string& operation) {
    
    // Check for deadlock and handle accordingly
    if (transaction_manager_ &&
        (transaction_manager_->detectDeadlock() || std::string(e.what()).find("deadlock") != std::string::npos)) {
        
        if (transaction_manager_->isInTransaction()) {
            try {
                transaction_manager_->rollbackTransaction();
            } catch (...) {
                // Ignore errors during rollback
            }
        }
    }
}

void MariaDBModule::logTransactionEvent(const std::string& event, const std::string& context) {
}

//=============================================================================
// Phase 4: Transaction Management Implementation
//=============================================================================

//=============================================================================
// Savepoint Implementation
//=============================================================================

Savepoint::Savepoint(const std::string& name, DatabaseConnection* connection)
    : name_(name)
    , connection_(connection)
    , is_active_(false)
    , created_at_(std::chrono::steady_clock::now())
{
    if (!connection_) {
        throw TransactionException("Invalid connection provided to Savepoint");
    }
    
    if (name_.empty()) {
        throw TransactionException("Savepoint name cannot be empty");
    }
    
    SecurityValidator::validateIdentifier(name_);
}

Savepoint::~Savepoint() {
    cleanup();
}

Savepoint::Savepoint(Savepoint&& other) noexcept
    : name_(std::move(other.name_))
    , connection_(other.connection_)
    , is_active_(other.is_active_)
    , created_at_(other.created_at_)
{
    other.connection_ = nullptr;
    other.is_active_ = false;
}

Savepoint& Savepoint::operator=(Savepoint&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        name_ = std::move(other.name_);
        connection_ = other.connection_;
        is_active_ = other.is_active_;
        created_at_ = other.created_at_;
        
        other.connection_ = nullptr;
        other.is_active_ = false;
    }
    return *this;
}

bool Savepoint::create() {
    if (!connection_ || !connection_->isConnected()) {
        throw TransactionException("No valid connection for savepoint creation");
    }
    
    if (is_active_) {
        return true;
    }
    
    std::string query = "SAVEPOINT `" + name_ + "`";
    
    try {
        bool success = connection_->executeNonQuery(query);
        if (success) {
            is_active_ = true;
            created_at_ = std::chrono::steady_clock::now();
        }
        return success;
    } catch (const std::exception& e) {
        throw TransactionException("Failed to create savepoint '" + name_ + "': " + e.what());
    }
}

bool Savepoint::rollbackTo() {
    if (!connection_ || !connection_->isConnected()) {
        throw TransactionException("No valid connection for savepoint rollback");
    }
    
    if (!is_active_) {
        throw TransactionException("Savepoint '" + name_ + "' is not active");
    }
    
    std::string query = "ROLLBACK TO SAVEPOINT `" + name_ + "`";
    
    try {
        bool success = connection_->executeNonQuery(query);
        if (success) {
        }
        return success;
    } catch (const std::exception& e) {
        throw TransactionException("Failed to rollback to savepoint '" + name_ + "': " + e.what());
    }
}

bool Savepoint::release() {
    if (!connection_ || !connection_->isConnected()) {
        throw TransactionException("No valid connection for savepoint release");
    }
    
    if (!is_active_) {
        return true; // Already released
    }
    
    std::string query = "RELEASE SAVEPOINT `" + name_ + "`";
    
    try {
        bool success = connection_->executeNonQuery(query);
        if (success) {
            is_active_ = false;
        }
        return success;
    } catch (const std::exception& e) {
        throw TransactionException("Failed to release savepoint '" + name_ + "': " + e.what());
    }
}

void Savepoint::cleanup() {
    if (is_active_ && connection_ && connection_->isConnected()) {
        try {
            release();
        } catch (...) {
            // Ignore errors during cleanup
        }
    }
    is_active_ = false;
}

//=============================================================================
// TransactionScope Implementation
//=============================================================================

TransactionScope::TransactionScope(DatabaseConnection* connection, bool auto_rollback)
    : connection_(connection)
    , transaction_active_(false)
    , auto_rollback_enabled_(auto_rollback)
    , committed_(false)
{
    if (!connection_) {
        throw TransactionException("Invalid connection provided to TransactionScope");
    }
}

TransactionScope::~TransactionScope() {
    std::lock_guard<std::mutex> lock(scope_mutex_);
    
    if (transaction_active_ && !committed_ && auto_rollback_enabled_) {
        try {
            rollback();
        } catch (...) {
            // Ignore errors during destruction
        }
    }
    
    cleanup();
}

bool TransactionScope::begin() {
    std::lock_guard<std::mutex> lock(scope_mutex_);
    
    if (transaction_active_) {
        return true;
    }
    
    if (!connection_ || !connection_->isConnected()) {
        throw TransactionException("No valid connection for transaction begin");
    }
    
    try {
        bool success = connection_->executeNonQuery("START TRANSACTION");
        if (success) {
            transaction_active_ = true;
            committed_ = false;
        }
        return success;
    } catch (const std::exception& e) {
        throw TransactionException("Failed to begin transaction: " + std::string(e.what()));
    }
}

bool TransactionScope::commit() {
    std::lock_guard<std::mutex> lock(scope_mutex_);
    
    if (!transaction_active_) {
        throw TransactionException("No active transaction to commit");
    }
    
    if (committed_) {
        return true;
    }
    
    try {
        // Release all savepoints before commit
        while (!savepoint_stack_.empty()) {
            savepoint_stack_.top()->release();
            savepoint_stack_.pop();
        }
        
        bool success = connection_->executeNonQuery("COMMIT");
        if (success) {
            committed_ = true;
            transaction_active_ = false;
        }
        return success;
    } catch (const std::exception& e) {
        throw TransactionException("Failed to commit transaction: " + std::string(e.what()));
    }
}

bool TransactionScope::rollback() {
    std::lock_guard<std::mutex> lock(scope_mutex_);
    
    if (!transaction_active_) {
        return true;
    }
    
    if (committed_) {
        throw TransactionException("Cannot rollback a committed transaction");
    }
    
    try {
        // Clear savepoint stack
        while (!savepoint_stack_.empty()) {
            savepoint_stack_.pop();
        }
        
        bool success = connection_->executeNonQuery("ROLLBACK");
        if (success) {
            transaction_active_ = false;
        }
        return success;
    } catch (const std::exception& e) {
        throw TransactionException("Failed to rollback transaction: " + std::string(e.what()));
    }
}

std::string TransactionScope::createSavepoint(const std::string& name) {
    std::lock_guard<std::mutex> lock(scope_mutex_);
    
    if (!transaction_active_) {
        throw TransactionException("Cannot create savepoint outside of transaction");
    }
    
    std::string savepoint_name = name.empty() ? generateSavepointName() : name;
    
    auto savepoint = std::make_unique<Savepoint>(savepoint_name, connection_);
    if (savepoint->create()) {
        savepoint_stack_.push(std::move(savepoint));
        return savepoint_name;
    }
    
    throw TransactionException("Failed to create savepoint: " + savepoint_name);
}

bool TransactionScope::rollbackToSavepoint(const std::string& name) {
    std::lock_guard<std::mutex> lock(scope_mutex_);
    
    if (!transaction_active_) {
        throw TransactionException("Cannot rollback to savepoint outside of transaction");
    }
    
    // Find and rollback to the specified savepoint
    std::stack<std::unique_ptr<Savepoint>> temp_stack;
    bool found = false;
    
    while (!savepoint_stack_.empty()) {
        auto& savepoint = savepoint_stack_.top();
        if (savepoint->getName() == name) {
            found = true;
            bool success = savepoint->rollbackTo();
            
            // Remove savepoints created after this one
            while (!temp_stack.empty()) {
                temp_stack.pop();
            }
            
            return success;
        }
        
        temp_stack.push(std::move(savepoint));
        savepoint_stack_.pop();
    }
    
    // Restore the stack if savepoint not found
    while (!temp_stack.empty()) {
        savepoint_stack_.push(std::move(temp_stack.top()));
        temp_stack.pop();
    }
    
    if (!found) {
        throw TransactionException("Savepoint not found: " + name);
    }
    
    return false;
}

bool TransactionScope::releaseSavepoint(const std::string& name) {
    std::lock_guard<std::mutex> lock(scope_mutex_);
    
    if (!transaction_active_) {
        throw TransactionException("Cannot release savepoint outside of transaction");
    }
    
    // Find and release the specified savepoint
    std::stack<std::unique_ptr<Savepoint>> temp_stack;
    bool found = false;
    
    while (!savepoint_stack_.empty()) {
        auto& savepoint = savepoint_stack_.top();
        if (savepoint->getName() == name) {
            found = true;
            bool success = savepoint->release();
            savepoint_stack_.pop();
            
            // Restore remaining savepoints
            while (!temp_stack.empty()) {
                savepoint_stack_.push(std::move(temp_stack.top()));
                temp_stack.pop();
            }
            
            return success;
        }
        
        temp_stack.push(std::move(savepoint));
        savepoint_stack_.pop();
    }
    
    // Restore the stack if savepoint not found
    while (!temp_stack.empty()) {
        savepoint_stack_.push(std::move(temp_stack.top()));
        temp_stack.pop();
    }
    
    if (!found) {
        throw TransactionException("Savepoint not found: " + name);
    }
    
    return false;
}

std::string TransactionScope::generateSavepointName() {
    static std::atomic<int> savepoint_counter{1};
    
    std::stringstream ss;
    ss << "sp_" << savepoint_counter.fetch_add(1) << "_";
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    ss << std::put_time(std::localtime(&time_t), "%H%M%S");
    
    return ss.str();
}

void TransactionScope::cleanup() {
    // Clear all savepoints
    while (!savepoint_stack_.empty()) {
        savepoint_stack_.pop();
    }
}

//=============================================================================
// TransactionManager Implementation
//=============================================================================

TransactionManager::TransactionManager(DatabaseConnection* connection)
    : connection_(connection)
    , transaction_active_(false)
    , auto_rollback_enabled_(true)
    , current_isolation_level_("REPEATABLE READ")
    , auto_commit_enabled_(true)
    , deadlock_timeout_(std::chrono::milliseconds(30000))
    , max_retry_attempts_(3)
    , retry_backoff_base_(std::chrono::milliseconds(100))
    , transaction_count_(0)
    , rollback_count_(0)
    , deadlock_count_(0)
{
    if (!connection_) {
        throw TransactionException("Invalid connection provided to TransactionManager");
    }
}

TransactionManager::~TransactionManager() {
    if (transaction_active_.load() && auto_rollback_enabled_) {
        try {
            rollbackTransaction();
        } catch (...) {
            // Ignore errors during destruction
        }
    }
}

bool TransactionManager::beginTransaction() {
    std::lock_guard<std::mutex> lock(transaction_mutex_);
    
    if (transaction_active_.load()) {
        return true;
    }
    
    if (!connection_ || !connection_->isConnected()) {
        throw TransactionException("No valid connection for transaction begin");
    }
    
    try {
        bool success = connection_->executeNonQuery("START TRANSACTION");
        if (success) {
            transaction_active_ = true;
            transaction_count_.fetch_add(1);
        }
        return success;
    } catch (const std::exception& e) {
        throw TransactionException("Failed to begin transaction: " + std::string(e.what()));
    }
}

bool TransactionManager::commitTransaction() {
    std::lock_guard<std::mutex> lock(transaction_mutex_);
    
    if (!transaction_active_.load()) {
        throw TransactionException("No active transaction to commit");
    }
    
    try {
        // Release all savepoints before commit
        while (!savepoint_stack_.empty()) {
            std::string savepoint_name = savepoint_stack_.top();
            savepoint_stack_.pop();
            
            std::string query = "RELEASE SAVEPOINT `" + savepoint_name + "`";
            connection_->executeNonQuery(query);
        }
        
        bool success = connection_->executeNonQuery("COMMIT");
        if (success) {
            transaction_active_ = false;
            updateStatistics(true);
        }
        return success;
    } catch (const std::exception& e) {
        updateStatistics(false);
        throw TransactionException("Failed to commit transaction: " + std::string(e.what()));
    }
}

bool TransactionManager::rollbackTransaction() {
    std::lock_guard<std::mutex> lock(transaction_mutex_);
    
    if (!transaction_active_.load()) {
        return true;
    }
    
    try {
        // Clear savepoint stack
        while (!savepoint_stack_.empty()) {
            savepoint_stack_.pop();
        }
        
        bool success = connection_->executeNonQuery("ROLLBACK");
        if (success) {
            transaction_active_ = false;
            updateStatistics(false);
        }
        return success;
    } catch (const std::exception& e) {
        updateStatistics(false);
        throw TransactionException("Failed to rollback transaction: " + std::string(e.what()));
    }
}

std::string TransactionManager::createSavepoint(const std::string& name) {
    std::lock_guard<std::mutex> lock(transaction_mutex_);
    
    if (!transaction_active_.load()) {
        throw TransactionException("Cannot create savepoint outside of transaction");
    }
    
    std::string savepoint_name = name.empty() ? generateSavepointName() : name;
    
    SecurityValidator::validateIdentifier(savepoint_name);
    
    std::string query = "SAVEPOINT `" + savepoint_name + "`";
    
    try {
        bool success = connection_->executeNonQuery(query);
        if (success) {
            savepoint_stack_.push(savepoint_name);
            return savepoint_name;
        }
        throw TransactionException("Failed to create savepoint: " + savepoint_name);
    } catch (const std::exception& e) {
        throw TransactionException("Failed to create savepoint '" + savepoint_name + "': " + e.what());
    }
}

bool TransactionManager::rollbackToSavepoint(const std::string& savepoint_name) {
    std::lock_guard<std::mutex> lock(transaction_mutex_);
    
    if (!transaction_active_.load()) {
        throw TransactionException("Cannot rollback to savepoint outside of transaction");
    }
    
    SecurityValidator::validateIdentifier(savepoint_name);
    
    std::string query = "ROLLBACK TO SAVEPOINT `" + savepoint_name + "`";
    
    try {
        bool success = connection_->executeNonQuery(query);
        if (success) {
            // Remove savepoints created after this one
            std::stack<std::string> temp_stack;
            while (!savepoint_stack_.empty() && savepoint_stack_.top() != savepoint_name) {
                temp_stack.push(savepoint_stack_.top());
                savepoint_stack_.pop();
            }
            
        }
        return success;
    } catch (const std::exception& e) {
        throw TransactionException("Failed to rollback to savepoint '" + savepoint_name + "': " + e.what());
    }
}

bool TransactionManager::releaseSavepoint(const std::string& savepoint_name) {
    std::lock_guard<std::mutex> lock(transaction_mutex_);
    
    if (!transaction_active_.load()) {
        throw TransactionException("Cannot release savepoint outside of transaction");
    }
    
    SecurityValidator::validateIdentifier(savepoint_name);
    
    std::string query = "RELEASE SAVEPOINT `" + savepoint_name + "`";
    
    try {
        bool success = connection_->executeNonQuery(query);
        if (success) {
            // Remove savepoint from stack
            std::stack<std::string> temp_stack;
            bool found = false;
            
            while (!savepoint_stack_.empty()) {
                if (savepoint_stack_.top() == savepoint_name) {
                    savepoint_stack_.pop();
                    found = true;
                    break;
                }
                temp_stack.push(savepoint_stack_.top());
                savepoint_stack_.pop();
            }
            
            // Restore remaining savepoints
            while (!temp_stack.empty()) {
                savepoint_stack_.push(temp_stack.top());
                temp_stack.pop();
            }
            
            if (found) {
            }
        }
        return success;
    } catch (const std::exception& e) {
        throw TransactionException("Failed to release savepoint '" + savepoint_name + "': " + e.what());
    }
}

size_t TransactionManager::getSavepointCount() const {
    std::lock_guard<std::mutex> lock(transaction_mutex_);
    return savepoint_stack_.size();
}

bool TransactionManager::setIsolationLevel(IsolationLevel level) {
    std::string level_str = isolationLevelToString(level);
    std::string query = "SET TRANSACTION ISOLATION LEVEL " + level_str;
    
    try {
        bool success = connection_->executeNonQuery(query);
        if (success) {
            current_isolation_level_ = level_str;
        }
        return success;
    } catch (const std::exception& e) {
        throw TransactionException("Failed to set isolation level: " + std::string(e.what()));
    }
}

TransactionManager::IsolationLevel TransactionManager::getIsolationLevel() const {
    return stringToIsolationLevel(current_isolation_level_);
}

bool TransactionManager::setAutoCommit(bool enabled) {
    std::string query = "SET AUTOCOMMIT = " + std::string(enabled ? "1" : "0");
    
    try {
        bool success = connection_->executeNonQuery(query);
        if (success) {
            auto_commit_enabled_ = enabled;
        }
        return success;
    } catch (const std::exception& e) {
        throw TransactionException("Failed to set auto-commit: " + std::string(e.what()));
    }
}

bool TransactionManager::detectDeadlock() {
    // Enhanced deadlock detection with multiple approaches
    bool deadlock_detected = false;
    
    try {
        // Approach 1: Check for deadlock-specific error codes
        std::string error_msg = connection_->getError();
        int error_code = mysql_errno(connection_->getHandle());
        
        // MySQL deadlock error codes
        if (error_code == 1213 || error_code == 1205) { // ER_LOCK_DEADLOCK, ER_LOCK_WAIT_TIMEOUT
            deadlock_detected = true;
        }
        
        // Approach 2: Check error message for deadlock keywords
        if (!deadlock_detected && !error_msg.empty()) {
            std::string lower_error = error_msg;
            std::transform(lower_error.begin(), lower_error.end(), lower_error.begin(), ::tolower);
            if (lower_error.find("deadlock") != std::string::npos ||
                lower_error.find("lock wait timeout") != std::string::npos) {
                deadlock_detected = true;
            }
        }
        
        // Approach 3: Check InnoDB engine status if no direct error
        if (!deadlock_detected) {
            std::string query = "SHOW ENGINE INNODB STATUS";
            MYSQL_RES* result = connection_->executeQuery(query);
            if (result) {
                MYSQL_ROW row = mysql_fetch_row(result);
                if (row && row[2]) {
                    std::string status(row[2]);
                    // Look for recent deadlock information in InnoDB status
                    if (status.find("LATEST DETECTED DEADLOCK") != std::string::npos ||
                        status.find("DEADLOCK") != std::string::npos) {
                        // Additional check: look for recent timestamp
                        auto now = std::chrono::system_clock::now();
                        auto recent_threshold = now - std::chrono::seconds(30); // 30 seconds threshold
                        deadlock_detected = true; // Simplified for now
                    }
                }
                mysql_free_result(result);
            }
        }
        
        // Update statistics if deadlock detected
        if (deadlock_detected) {
            deadlock_count_.fetch_add(1);
            
            // Trigger deadlock resolution if needed
            handleDeadlock();
        }
        
        return deadlock_detected;
        
    } catch (const std::exception& e) {
        // If we can't check for deadlock, assume no deadlock but log the issue
        return false;
    }
}

bool TransactionManager::executeWithRetry(std::function<bool()> operation, int max_retries) {
    int attempts = max_retries < 0 ? max_retry_attempts_ : max_retries;
    
    for (int attempt = 0; attempt < attempts; ++attempt) {
        try {
            bool success = operation();
            if (success) {
                return true;
            }
        } catch (const TransactionException& e) {
            if (detectDeadlock() || std::string(e.what()).find("deadlock") != std::string::npos) {
                if (attempt < attempts - 1) {
                    
                    // Rollback current transaction before retry
                    if (transaction_active_.load()) {
                        rollbackTransaction();
                    }
                    
                    // Exponential backoff
                    std::this_thread::sleep_for(calculateBackoff(attempt));
                    continue;
                }
            }
            throw; // Re-throw if not deadlock or out of retries
        }
    }
    
    return false;
}

void TransactionManager::resetStatistics() {
    transaction_count_ = 0;
    rollback_count_ = 0;
    deadlock_count_ = 0;
}

std::string TransactionManager::generateSavepointName() {
    static std::atomic<int> savepoint_counter{1};
    
    std::stringstream ss;
    ss << "sp_" << savepoint_counter.fetch_add(1) << "_";
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    ss << std::put_time(std::localtime(&time_t), "%H%M%S");
    
    return ss.str();
}

std::string TransactionManager::isolationLevelToString(IsolationLevel level) const {
    switch (level) {
        case IsolationLevel::READ_UNCOMMITTED:
            return "READ UNCOMMITTED";
        case IsolationLevel::READ_COMMITTED:
            return "READ COMMITTED";
        case IsolationLevel::REPEATABLE_READ:
            return "REPEATABLE READ";
        case IsolationLevel::SERIALIZABLE:
            return "SERIALIZABLE";
        default:
            return "REPEATABLE READ";
    }
}

TransactionManager::IsolationLevel TransactionManager::stringToIsolationLevel(const std::string& level) const {
    if (level == "READ UNCOMMITTED") {
        return IsolationLevel::READ_UNCOMMITTED;
    } else if (level == "READ COMMITTED") {
        return IsolationLevel::READ_COMMITTED;
    } else if (level == "REPEATABLE READ") {
        return IsolationLevel::REPEATABLE_READ;
    } else if (level == "SERIALIZABLE") {
        return IsolationLevel::SERIALIZABLE;
    } else {
        return IsolationLevel::REPEATABLE_READ;
    }
}

void TransactionManager::updateStatistics(bool committed, bool deadlock_detected) {
    if (!committed) {
        rollback_count_.fetch_add(1);
    }
    if (deadlock_detected) {
        deadlock_count_.fetch_add(1);
    }
}

std::chrono::milliseconds TransactionManager::calculateBackoff(int attempt) const {
    // Exponential backoff with jitter
    auto base_delay = retry_backoff_base_.count();
    auto delay = base_delay * (1 << attempt); // 2^attempt
    
    // Add random jitter (±25%)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.75, 1.25);
    
    delay = static_cast<long>(delay * dis(gen));
    
    return std::chrono::milliseconds(delay);
}

bool TransactionManager::handleDeadlock() {
    deadlock_count_.fetch_add(1);
    
    // Rollback current transaction
    if (transaction_active_.load()) {
        rollbackTransaction();
    }
    
    return true;
}

} // namespace Modules
