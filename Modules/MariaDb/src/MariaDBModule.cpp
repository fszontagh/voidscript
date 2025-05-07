// MariaDBModule.cpp
#include "MariaDBModule.hpp"

#include <mariadb/mysql.h>

#include <stdexcept>
#include <string>

#include "Modules/ModuleManager.hpp"
#include "Symbols/ClassRegistry.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

// Map connection handles to MYSQL pointers
static std::map<int, MYSQL *> connMap;
static int                    nextConnId = 1;

void MariaDBModule::registerModule(IModuleContext & context) {
    // Register MariaDB class and its methods
    auto & registry = Symbols::ClassRegistry::instance();
    registry.registerClass(this->name());
    registry.addMethod(this->name(), "connect");
    registry.addMethod(this->name(), "query");
    registry.addMethod(this->name(), "close");

    // shrtcut helpers
    registry.addMethod(this->name(), "insert");

    //const std::vector<FunctParameterInfo> paramList = { "host", "user", "pass", "db" };
    const std::vector<FunctParameterInfo> paramList = {
        { "host", Symbols::Variables::Type::STRING, false },
        { "user", Symbols::Variables::Type::STRING, false },
        { "pass", Symbols::Variables::Type::STRING, false },
        { "db",   Symbols::Variables::Type::STRING, false },
    };

    REGISTER_MODULE_FUNCTION(context, this->name(), "connect", Symbols::Variables::Type::CLASS, paramList,
                             "Connect to MariaDB",
                             [this](const std::vector<Symbols::Value> & args) { return this->connect(args); });


                             paramList = {
                                { "query", Symbols::Variables::Type::STRING, false }
                            };

    REGISTER_MODULE_FUNCTION(context, this->name(), "query", Symbols::Variables::Type::OBJECT, paramList,
                             "Execute MariaDB query",
                             [this](const std::vector<Symbols::Value> & args) { return this->query(args); });

    REGISTER_MODULE_FUNCTION(context, this->name(), "close", Symbols::Variables::Type::NULL_TYPE, {},
                             "Close MariaDB connection",
                             [this](const std::vector<Symbols::Value> & args) { return this->close(args); });

    // Register native callbacks for class methods
    /*auto & mgr = ModuleManager::instance();
    mgr.registerFunction(
        "MariaDB::connect", [this](const std::vector<Value> & args) { return this->connect(args); },
        Symbols::Variables::Type::CLASS);
    mgr.registerFunction(
        "MariaDB::query", [this](const std::vector<Value> & args) { return this->query(args); },
        Symbols::Variables::Type::OBJECT);
    mgr.registerFunction(
        "MariaDB::close", [this](const std::vector<Value> & args) { return this->close(args); },
        Symbols::Variables::Type::NULL_TYPE);

    mgr.registerFunction(
        "MariaDB::insert", [this](const std::vector<Value> & args) { return this->insert(args); },
        Symbols::Variables::Type::INTEGER);*/
}

Symbols::Value MariaDBModule::connect(FunctionArguments & args) {
    using namespace Symbols;
    if (args.size() != 5) {
        throw std::runtime_error("MariaDB::connect expects (host, user, pass, db), got: " +
                                 std::to_string(args.size() - 1));
    }
    // Extract object instance map
    if (args[0].getType() != Variables::Type::CLASS && args[0].getType() != Variables::Type::OBJECT) {
        throw std::runtime_error("MariaDB::connect must be called on MariaDB instance");
    }
    auto        objMap = std::get<Value::ObjectMap>(args[0].get());
    // Connection parameters
    std::string host   = Value::to_string(args[1].get());
    std::string user   = Value::to_string(args[2].get());
    std::string pass   = Value::to_string(args[3].get());
    std::string db     = Value::to_string(args[4].get());
    // Initialize and connect
    MYSQL *     conn   = mysql_init(nullptr);
    if (!conn) {
        throw std::runtime_error("MariaDB: mysql_init failed");
    }
    if (!mysql_real_connect(conn, host.c_str(), user.c_str(), pass.c_str(), db.c_str(), 0, nullptr, 0)) {
        std::string err = mysql_error(conn);
        mysql_close(conn);
        throw std::runtime_error("MariaDB connect failed: " + err);
    }
    // Store connection handle
    int handle            = nextConnId++;
    connMap[handle]       = conn;
    objMap["__conn_id__"] = Value(handle);
    return Value::makeClassInstance(objMap);
}

Symbols::Value MariaDBModule::query(FunctionArguments & args) {
    using namespace Symbols;
    if (args.size() < 2) {
        throw std::runtime_error("MariaDB::query expects (this, sql)");
    }
    auto objVal = args[0];
    if (objVal.getType() != Variables::Type::CLASS && objVal.getType() != Variables::Type::OBJECT) {
        throw std::runtime_error("MariaDB::query must be called on MariaDB instance");
    }
    auto objMap   = std::get<Value::ObjectMap>(objVal.get());
    // Get connection handle
    int  handle   = 0;
    auto itHandle = objMap.find("__conn_id__");
    if (itHandle == objMap.end() || itHandle->second.getType() != Variables::Type::INTEGER) {
        throw std::runtime_error("MariaDB query: no valid connection");
    }
    handle      = itHandle->second.get<int>();
    auto connIt = connMap.find(handle);
    if (connIt == connMap.end()) {
        throw std::runtime_error("MariaDB query: connection not found");
    }
    MYSQL *     conn = connIt->second;
    std::string sql  = Value::to_string(args[1].get());

    if (mysql_query(conn, sql.c_str())) {
        throw std::runtime_error(std::string("MariaDB query failed: ") + mysql_error(conn));
    }
    MYSQL_RES * res = mysql_store_result(conn);
    if (!res) {
        // No result set or error
        //if (mysql_field_count(conn) == 0) {
        //    return Value(Value::ObjectMap{});
        // }
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
    Value::ObjectMap result;
    MYSQL_ROW        row;
    unsigned long *  lengths;
    int              rowIndex = 0;
    while ((row = mysql_fetch_row(res))) {
        lengths = mysql_fetch_lengths(res);
        Value::ObjectMap rowObj;
        for (unsigned int i = 0; i < num_fields; ++i) {
            std::string val       = row[i] ? std::string(row[i], lengths[i]) : std::string();
            rowObj[fieldNames[i]] = Value(val);
        }
        result[std::to_string(rowIndex++)] = Value(rowObj);
    }
    mysql_free_result(res);
    return result;
}

Symbols::Value MariaDBModule::close(FunctionArguments & args) {
    using namespace Symbols;
    if (args.size() < 1) {
        throw std::runtime_error("MariaDB::close expects (this)");
    }
    auto objVal = args[0];
    if (objVal.getType() != Variables::Type::CLASS && objVal.getType() != Variables::Type::OBJECT) {
        throw std::runtime_error("MariaDB::close must be called on MariaDB instance");
    }
    auto objMap   = std::get<Value::ObjectMap>(objVal.get());
    auto itHandle = objMap.find("__conn_id__");
    if (itHandle != objMap.end() && itHandle->second.getType() == Variables::Type::INTEGER) {
        int  handle = itHandle->second.get<int>();
        auto connIt = connMap.find(handle);
        if (connIt != connMap.end()) {
            mysql_close(connIt->second);
            connMap.erase(connIt);
        }
    }
    return Value::makeNull(Variables::Type::NULL_TYPE);
}

Symbols::Value MariaDBModule::insert(FunctionArguments & args) {
    if (args.size() < 3) {
        throw std::invalid_argument("MariaDB::insert expects table_name, object");
    }
    if (args[1].getType() != Symbols::Variables::Type::STRING) {
        throw std::invalid_argument("First parameter is not string");
    }
    if (args[2].getType() != Symbols::Variables::Type::OBJECT) {
        throw std::invalid_argument("Second parameter needs object, object array");
    }
    std::string query = "INSERT INTO `" + Symbols::Value::to_string(args[1]) + "` VALUES ()";
    std::cout << "QUERY: " << query << "\n";
    return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
}

}  // namespace Modules
