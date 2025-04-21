project(
    MariaDbModule
    VERSION 0.0.1
    DESCRIPTION "MariaDB support module for VoidScript"
    LANGUAGES CXX
)

# Find MariaDB headers and client library
find_path(MARIADB_INCLUDE_DIR NAMES mysql.h PATH_SUFFIXES mariadb mysql)
find_library(MARIADBCLIENT_LIB NAMES mariadb mariadbclient mysqlclient)

if (NOT MARIADB_INCLUDE_DIR OR NOT MARIADBCLIENT_LIB)
    message(WARNING "MariaDB client library or header not found; skipping MariaDbModule")
else()
    add_library(MariaDbModule SHARED)
    target_sources(MariaDbModule
        PRIVATE
            src/MariaDBModule.cpp
            src/PluginInit.cpp
    )
    target_include_directories(MariaDbModule PRIVATE ${MARIADB_INCLUDE_DIR})
    target_link_libraries(MariaDbModule PRIVATE voidscript ${MARIADBCLIENT_LIB})
endif()
