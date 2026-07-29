#ifndef SQLITE_MODULE_HPP
#define SQLITE_MODULE_HPP

#include <sqlite3.h>

#include <string>
#include <unordered_map>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

/**
 * @brief VoidScript class "SQLite" wrapping libsqlite3 - a zero-config embedded database.
 *
 *   SQLite $db = new SQLite();
 *   $db->open("/tmp/app.db");                 // or ":memory:"
 *   $db->exec("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT)");
 *   $db->exec("INSERT INTO t (name) VALUES (?)", ["alice"]);   // prepared / bound
 *   int $id = $db->lastInsertId();
 *   auto $rows = $db->query("SELECT * FROM t WHERE id = ?", [$id]);
 *   printnl($rows[0]->name);
 *   $db->close();
 *
 * Each instance owns its own sqlite3* handle, keyed by the framework instance id.
 */
class SQLiteModule : public BaseModule {
  public:
    SQLiteModule() {
        setModuleName("SQLite");
        setDescription("Zero-config embedded SQL database via libsqlite3, with prepared-statement parameter binding");
    }

    ~SQLiteModule() override;

    void registerFunctions() override;

  private:
    std::unordered_map<long, sqlite3 *> dbs_;

    Symbols::ValuePtr construct(FunctionArguments & args);
    Symbols::ValuePtr open(FunctionArguments & args);
    Symbols::ValuePtr isOpen(FunctionArguments & args);
    Symbols::ValuePtr close(FunctionArguments & args);
    Symbols::ValuePtr exec(FunctionArguments & args);
    Symbols::ValuePtr query(FunctionArguments & args);
    Symbols::ValuePtr lastInsertId(FunctionArguments & args);
    Symbols::ValuePtr changes(FunctionArguments & args);

    sqlite3 * dbFor(FunctionArguments & args, const char * method);
    void      bindParams(sqlite3_stmt * stmt, FunctionArguments & args, size_t paramArgIndex, const char * method);
};

}  // namespace Modules

#endif  // SQLITE_MODULE_HPP
