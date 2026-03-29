if (NOT BUILD_MODULE_MARIADB)
    return()
endif()
include(module.cmake)
project(
    modules-${MODULE_NAME}
    VERSION 0.0.1
    DESCRIPTION "${MODULE_DESCRIPTION}"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)

# Find MariaDB headers and client library
find_path(MARIADB_INCLUDE_DIR NAMES mysql.h PATH_SUFFIXES mariadb mysql)
find_library(MARIADBCLIENT_LIB NAMES mariadb mariadbclient mysqlclient)

if (NOT MARIADB_INCLUDE_DIR OR NOT MARIADBCLIENT_LIB)
    message(FATAL "MariaDB client library or header not found")
endif()

add_library(voidscript-module-${MODULE_NAME} SHARED)
target_sources(voidscript-module-${MODULE_NAME}
    PRIVATE
        src/MariaDBModule.cpp
        src/PluginInit.cpp
)

target_include_directories(voidscript-module-${MODULE_NAME} PRIVATE ${MARIADB_INCLUDE_DIR})
target_link_libraries(voidscript-module-${MODULE_NAME} PRIVATE voidscript ${MARIADBCLIENT_LIB})

install(TARGETS voidscript-module-${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT "modules-${MODULE_NAME}")
