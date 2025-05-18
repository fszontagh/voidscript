// MariaDBModule.cpp
#include "MariaDBModule.hpp"

#include <mariadb/mysql.h>

#include <stdexcept>
#include <string>

#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

// Map connection handles to MYSQL pointers
static std::map<int, MYSQL *> connMap;
static int                    nextConnId = 1;

void MariaDBModule::registerModule() {
    // Register MariaDB class and its methods
    REGISTER_CLASS(this->name());

    std::vector<FunctParameterInfo> params = {
        { "host", Symbols::Variables::Type::STRING, "Database host to connect",  false },
        { "user", Symbols::Variables::Type::STRING, "Username to authnenticate", false },
        { "pass", Symbols::Variables::Type::STRING, "Password to authenticate",  false },
        { "db",   Symbols::Variables::Type::STRING, "Database name",             false },
    };

    REGISTER_METHOD(
        this->name(), "connect", params, [this](const FunctionArguments & args) { return this->connect(args); },
        Symbols::Variables::Type::CLASS, "Connect to MariaDB host");

    params = {
        { "query_string", Symbols::Variables::Type::STRING, "SQL query string to execute", false },
    };

    REGISTER_METHOD(
        this->name(), "query", params, [this](const FunctionArguments & args) { return this->query(args); },
        Symbols::Variables::Type::OBJECT, "Execute MariaDB query");

    REGISTER_METHOD(
        this->name(), "close", {}, [this](const FunctionArguments & args) { return this->close(args); },
        Symbols::Variables::Type::NULL_TYPE, "Close MariaDB connection");

    params = {
        { "data",       Symbols::Variables::Type::OBJECT, "INSERT INTO data",          false },
        { "table_name", Symbols::Variables::Type::STRING, "Table name to insert into", false },
    };

    REGISTER_METHOD(
        this->name(), "insert", params, [this](const FunctionArguments & args) { return this->insert(args); },
        Symbols::Variables::Type::INTEGER, "Insert data into MariaDB table");
}

Symbols::Value::ValuePtr MariaDBModule::connect(FunctionArguments & args) {
    if (args.size() != 5) {
        throw std::runtime_error("MariaDB::connect expects (host, user, pass, db), got: " +
                                 std::to_string(args.size() - 1));
    }
    // Extract object instance map
    if (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("MariaDB::connect must be called on MariaDB instance");
    }
    Symbols::Value::ObjectMap objMap = *args[0];  //->get<Symbols::Value::ObjectMap>();
    std::string               host   = *args[1];
    std::string               user   = *args[2];
    std::string               pass   = *args[3];
    std::string               db     = *args[4];
    // Initialize and connect
    MYSQL *                   conn   = mysql_init(nullptr);
    if (!conn) {
        throw std::runtime_error("MariaDB: mysql_init failed");
    }
    if (!mysql_real_connect(conn, host.c_str(), user.c_str(), pass.c_str(), db.c_str(), 0, nullptr, 0)) {
        std::string err = mysql_error(conn);
        mysql_close(conn);
        throw std::runtime_error("MariaDB connect failed: " + err);
    }
    // Store connection handle
    int handle      = nextConnId++;
    connMap[handle] = conn;

    // Use the new property management system
    auto & manager = UnifiedModuleManager::instance();
    manager.setObjectProperty(this->name(), "__conn_id__", Symbols::Value::create(handle));
    manager.setObjectProperty(this->name(), "__class__", Symbols::Value::create(name()));

    // Copy properties to the object map for backward compatibility
    objMap["__conn_id__"] = manager.getObjectProperty(this->name(), "__conn_id__");
    objMap["__class__"]   = manager.getObjectProperty(this->name(), "__class__");

    return Symbols::Value::makeClassInstance(objMap);
}

Symbols::Value::ValuePtr MariaDBModule::query(FunctionArguments & args) {
    if (args.size() < 2) {
        throw std::runtime_error("MariaDB::query expects (this, sql)");
    }
    const auto & objVal = args[0];
    if (objVal != Symbols::Variables::Type::CLASS && objVal != Symbols::Variables::Type::OBJECT) {
        throw std::runtime_error("MariaDB::query must be called on MariaDB instance");
    }
    Symbols::Value::ObjectMap objMap = *objVal;

    // Get connection handle using the new property management system
    auto & manager = UnifiedModuleManager::instance();
    if (!manager.hasObjectProperty(this->name(), "__conn_id__")) {
        throw std::runtime_error("MariaDB query: no valid connection");
    }
    int handle = *manager.getObjectProperty(this->name(), "__conn_id__");

    auto connIt = connMap.find(handle);
    if (connIt == connMap.end()) {
        throw std::runtime_error("MariaDB query: connection not found");
    }
    MYSQL *     conn = connIt->second;
    std::string sql  = *args[1];

    if (mysql_query(conn, sql.c_str())) {
        throw std::runtime_error(std::string("MariaDB query failed: ") + mysql_error(conn));
    }
    MYSQL_RES * res = mysql_store_result(conn);
    if (!res) {
        throw std::runtime_error(std::string("MariaDB query store_result failed: ") + mysql_error(conn));
    }
    // Fetch field names
    unsigned int             num_fields = mysql_num_fields(res);
    std::vector<std::string> fieldNames;
    MYSQL_FIELD *            field;
    for (unsigned int i = 0; i < num_fields; ++i) {
        field = mysql_fetch_field_direct(res, i);
        fieldNames.emplace_back(field->name);
    }
    // Collect rows into object map
    Symbols::Value::ObjectMap result;
    MYSQL_ROW                 row;
    unsigned long *           lengths;
    int                       rowIndex = 0;
    while ((row = mysql_fetch_row(res))) {
        lengths = mysql_fetch_lengths(res);
        Symbols::Value::ObjectMap rowObj;
        for (unsigned int i = 0; i < num_fields; ++i) {
            std::string val       = row[i] ? std::string(row[i], lengths[i]) : std::string();
            rowObj[fieldNames[i]] = Symbols::Value::create(val);
        }
        result[std::to_string(rowIndex++)] = Symbols::Value::create(rowObj);
    }
    mysql_free_result(res);
    return result;
}

Symbols::Value::ValuePtr MariaDBModule::close(FunctionArguments & args) {
    if (args.size() < 1) {
        throw std::runtime_error("MariaDB::close expects (this)");
    }
    auto objVal = args[0];
    if (objVal->getType() != Variables::Type::CLASS && objVal->getType() != Variables::Type::OBJECT) {
        throw std::runtime_error("MariaDB::close must be called on MariaDB instance");
    }
    auto objMap = std::get<Value::ObjectMap>(objVal.get());

    // Use the new property management system
    auto & manager = UnifiedModuleManager::instance();
    if (manager.hasObjectProperty(this->name(), "__conn_id__")) {
        int  handle = std::get<int>(manager.getObjectProperty(this->name(), "__conn_id__").get());
        auto connIt = connMap.find(handle);
        if (connIt != connMap.end()) {
            mysql_close(connIt->second);
            connMap.erase(connIt);
        }
        // Clear the connection property
        manager.deleteObjectProperty(this->name(), "__conn_id__");
    }
    return Value::makeNull(Variables::Type::NULL_TYPE);
}

Symbols::Value::ValuePtr MariaDBModule::insert(FunctionArguments & args) {
    if (args.size() < 3) {
        throw std::invalid_argument("MariaDB::insert expects table_name, object");
    }
    if (args[1]->getType() != Symbols::Variables::Type::STRING) {
        throw std::invalid_argument("First parameter is not string");
    }
    if (args[2]->getType() != Symbols::Variables::Type::OBJECT) {
        throw std::invalid_argument("Second parameter needs object, object array");
    }
    std::string query = "INSERT INTO `" + Symbols::Value::to_string(args[1]) + "` VALUES ()";
    std::cout << "QUERY: " << query << "\n";
    return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
}

}  // namespace Modules
