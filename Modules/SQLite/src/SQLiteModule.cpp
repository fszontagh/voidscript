#include "SQLiteModule.hpp"

#include <stdexcept>
#include <string>
#include <vector>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

SQLiteModule::~SQLiteModule() {
    for (auto & kv : dbs_) {
        if (kv.second) {
            sqlite3_close(kv.second);
        }
    }
}

void SQLiteModule::registerFunctions() {
    REGISTER_CLASS(this->name());

    REGISTER_METHOD(this->name(), "__construct", {},
                    [this](FunctionArguments & args) { return this->construct(args); },
                    Symbols::Variables::Type::CLASS, "Create a SQLite instance");

    std::vector<Symbols::FunctionParameterInfo> path_param = {
        { "path", Symbols::Variables::Type::STRING, "Database file path, or \":memory:\"" }
    };
    REGISTER_METHOD(this->name(), "open", path_param,
                    [this](FunctionArguments & args) { return this->open(args); },
                    Symbols::Variables::Type::BOOLEAN, "Open (or create) a database file");
    REGISTER_METHOD(this->name(), "isOpen", {},
                    [this](FunctionArguments & args) { return this->isOpen(args); },
                    Symbols::Variables::Type::BOOLEAN, "Whether a database is open");
    REGISTER_METHOD(this->name(), "close", {},
                    [this](FunctionArguments & args) { return this->close(args); },
                    Symbols::Variables::Type::NULL_TYPE, "Close the database");

    std::vector<Symbols::FunctionParameterInfo> sql_params = {
        { "sql", Symbols::Variables::Type::STRING, "SQL statement" },
        { "params", Symbols::Variables::Type::OBJECT, "Array of values bound to ? placeholders", true }
    };
    REGISTER_METHOD(this->name(), "exec", sql_params,
                    [this](FunctionArguments & args) { return this->exec(args); },
                    Symbols::Variables::Type::INTEGER,
                    "Run a write/DDL statement (optionally with bound params); returns changed rows");
    REGISTER_METHOD(this->name(), "query", sql_params,
                    [this](FunctionArguments & args) { return this->query(args); },
                    Symbols::Variables::Type::OBJECT,
                    "Run a SELECT (optionally with bound params); returns an array of row objects");

    REGISTER_METHOD(this->name(), "lastInsertId", {},
                    [this](FunctionArguments & args) { return this->lastInsertId(args); },
                    Symbols::Variables::Type::INTEGER, "Rowid of the most recent INSERT");
    REGISTER_METHOD(this->name(), "changes", {},
                    [this](FunctionArguments & args) { return this->changes(args); },
                    Symbols::Variables::Type::INTEGER, "Rows changed by the most recent statement");
}

Symbols::ValuePtr SQLiteModule::construct(FunctionArguments & args) {
    // Stateless until open(); nothing to stamp. `new` returns args[0].
    return args[0];
}

sqlite3 * SQLiteModule::dbFor(FunctionArguments & args, const char * method) {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error(std::string("SQLite::") + method + " must be called on a SQLite instance");
    }
    const long id = Symbols::ValuePtr::instanceId(args[0]);
    auto       it = dbs_.find(id);
    if (it == dbs_.end() || it->second == nullptr) {
        throw std::runtime_error(std::string("SQLite::") + method + ": no database open - call open() first");
    }
    return it->second;
}

Symbols::ValuePtr SQLiteModule::open(FunctionArguments & args) {
    if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("SQLite::open expects (string path)");
    }
    const long id = Symbols::ValuePtr::instanceId(args[0]);
    // A second open() on the same instance replaces the previous handle.
    auto existing = dbs_.find(id);
    if (existing != dbs_.end() && existing->second) {
        sqlite3_close(existing->second);
        existing->second = nullptr;
    }
    const std::string path = args[1]->get<std::string>();
    sqlite3 *         db   = nullptr;
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
        const std::string err = db ? sqlite3_errmsg(db) : "unknown error";
        if (db) {
            sqlite3_close(db);
        }
        throw std::runtime_error("SQLite::open failed for '" + path + "': " + err);
    }
    dbs_[id] = db;
    return Symbols::ValuePtr(true);
}

Symbols::ValuePtr SQLiteModule::isOpen(FunctionArguments & args) {
    const long id = Symbols::ValuePtr::instanceId(args[0]);
    auto       it = dbs_.find(id);
    return Symbols::ValuePtr(it != dbs_.end() && it->second != nullptr);
}

Symbols::ValuePtr SQLiteModule::close(FunctionArguments & args) {
    const long id = Symbols::ValuePtr::instanceId(args[0]);
    auto       it = dbs_.find(id);
    if (it != dbs_.end() && it->second) {
        sqlite3_close(it->second);
        dbs_.erase(it);
    }
    return Symbols::ValuePtr::null();
}

// Bind the values of the array at args[paramArgIndex] to ? placeholders (1-based).
void SQLiteModule::bindParams(sqlite3_stmt * stmt, FunctionArguments & args, size_t paramArgIndex,
                              const char * method) {
    if (args.size() <= paramArgIndex || args[paramArgIndex]->is_null()) {
        return;
    }
    if (args[paramArgIndex] != Symbols::Variables::Type::OBJECT &&
        args[paramArgIndex] != Symbols::Variables::Type::CLASS) {
        throw std::runtime_error(std::string("SQLite::") + method + ": params must be an array");
    }
    const Symbols::ObjectMap & map = args[paramArgIndex]->get<Symbols::ObjectMap>();
    for (size_t i = 0;; ++i) {
        auto it = map.find(std::to_string(i));
        if (it == map.end()) {
            break;
        }
        const int         idx = static_cast<int>(i) + 1;  // sqlite params are 1-based
        Symbols::ValuePtr v   = it->second;
        switch (v->getType()) {
            case Symbols::Variables::Type::INTEGER:
                sqlite3_bind_int64(stmt, idx, v->get<int>());
                break;
            case Symbols::Variables::Type::DOUBLE:
                sqlite3_bind_double(stmt, idx, v->get<double>());
                break;
            case Symbols::Variables::Type::FLOAT:
                sqlite3_bind_double(stmt, idx, static_cast<double>(v->get<float>()));
                break;
            case Symbols::Variables::Type::BOOLEAN:
                sqlite3_bind_int(stmt, idx, v->get<bool>() ? 1 : 0);
                break;
            case Symbols::Variables::Type::NULL_TYPE:
                sqlite3_bind_null(stmt, idx);
                break;
            default: {
                const std::string s = v->toString();
                sqlite3_bind_text(stmt, idx, s.c_str(), static_cast<int>(s.size()), SQLITE_TRANSIENT);
            }
        }
    }
}

Symbols::ValuePtr SQLiteModule::exec(FunctionArguments & args) {
    if (args.size() < 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("SQLite::exec expects (string sql [, array params])");
    }
    sqlite3 *         db  = dbFor(args, "exec");
    const std::string sql = args[1]->get<std::string>();

    // With no params, use sqlite3_exec so a multi-statement script (e.g. a schema) runs
    // in one call. With params, use a single prepared statement for safe binding.
    if (args.size() < 3 || args[2]->is_null()) {
        char * errmsg = nullptr;
        if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
            const std::string err = errmsg ? errmsg : "unknown error";
            sqlite3_free(errmsg);
            throw std::runtime_error("SQLite::exec failed: " + err);
        }
        return Symbols::ValuePtr(sqlite3_changes(db));
    }

    sqlite3_stmt * stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("SQLite::exec prepare failed: ") + sqlite3_errmsg(db));
    }
    bindParams(stmt, args, 2, "exec");
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        throw std::runtime_error(std::string("SQLite::exec failed: ") + sqlite3_errmsg(db));
    }
    return Symbols::ValuePtr(sqlite3_changes(db));
}

Symbols::ValuePtr SQLiteModule::query(FunctionArguments & args) {
    if (args.size() < 2 || args[1] != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("SQLite::query expects (string sql [, array params])");
    }
    sqlite3 *         db  = dbFor(args, "query");
    const std::string sql = args[1]->get<std::string>();

    sqlite3_stmt * stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("SQLite::query prepare failed: ") + sqlite3_errmsg(db));
    }
    bindParams(stmt, args, 2, "query");

    Symbols::ObjectMap rows;
    int                rowIndex = 0;
    int                rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Symbols::ObjectMap row;
        const int          cols = sqlite3_column_count(stmt);
        for (int c = 0; c < cols; ++c) {
            const char *      name = sqlite3_column_name(stmt, c);
            const std::string key  = name ? name : std::to_string(c);
            switch (sqlite3_column_type(stmt, c)) {
                case SQLITE_INTEGER:
                    row[key] = Symbols::ValuePtr(static_cast<int>(sqlite3_column_int64(stmt, c)));
                    break;
                case SQLITE_FLOAT:
                    row[key] = Symbols::ValuePtr(sqlite3_column_double(stmt, c));
                    break;
                case SQLITE_NULL:
                    row[key] = Symbols::ValuePtr::null();
                    break;
                case SQLITE_TEXT:
                case SQLITE_BLOB:
                default: {
                    const void * data = sqlite3_column_blob(stmt, c);
                    const int    n    = sqlite3_column_bytes(stmt, c);
                    row[key]          = Symbols::ValuePtr(std::string(static_cast<const char *>(data), n));
                    break;
                }
            }
        }
        rows[std::to_string(rowIndex++)] = Symbols::ValuePtr(row);
    }
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(std::string("SQLite::query failed: ") + sqlite3_errmsg(db));
    }
    return Symbols::ValuePtr(rows);
}

Symbols::ValuePtr SQLiteModule::lastInsertId(FunctionArguments & args) {
    sqlite3 * db = dbFor(args, "lastInsertId");
    return Symbols::ValuePtr(static_cast<int>(sqlite3_last_insert_rowid(db)));
}

Symbols::ValuePtr SQLiteModule::changes(FunctionArguments & args) {
    sqlite3 * db = dbFor(args, "changes");
    return Symbols::ValuePtr(sqlite3_changes(db));
}

}  // namespace Modules
