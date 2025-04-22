if (NOT BUILD_MODULE_MARIADB)
    return()
endif()

project(
    modules-mariadb
    VERSION 0.0.1
    DESCRIPTION "MariaDB support module for VoidScript"
    LANGUAGES CXX
)

# Find MariaDB headers and client library
find_path(MARIADB_INCLUDE_DIR NAMES mysql.h PATH_SUFFIXES mariadb mysql)
find_library(MARIADBCLIENT_LIB NAMES mariadb mariadbclient mysqlclient)

if (NOT MARIADB_INCLUDE_DIR OR NOT MARIADBCLIENT_LIB)
    message(FATAL "MariaDB client library or header not found")
endif()

    add_library(modules-mariadb SHARED)
    target_sources(modules-mariadb
        PRIVATE
            src/MariaDBModule.cpp
            src/PluginInit.cpp
    )
    target_include_directories(modules-mariadb PRIVATE ${MARIADB_INCLUDE_DIR})
    target_link_libraries(modules-mariadb PRIVATE voidscript ${MARIADBCLIENT_LIB})


install(TARGETS modules-mariadb DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT modules-mariadb)
