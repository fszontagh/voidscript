# Modules/SQLite/CMakeLists.txt
#
# Wraps libsqlite3 as the VoidScript class "SQLite": a zero-config embedded SQL database
# (no server), with prepared statements for safe parameter binding.

if (NOT BUILD_MODULE_SQLITE)
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

find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h)
find_library(SQLITE3_LIB NAMES sqlite3)

if (NOT SQLITE3_INCLUDE_DIR OR NOT SQLITE3_LIB)
    message(WARNING "SQLite module: libsqlite3 (sqlite3.h / libsqlite3) not found - skipping module")
    return()
endif()

add_library(voidscript-module-${MODULE_NAME} SHARED)
target_sources(voidscript-module-${MODULE_NAME}
    PRIVATE
        src/SQLiteModule.cpp
        src/PluginInit.cpp
)

target_include_directories(voidscript-module-${MODULE_NAME} PRIVATE ${SQLITE3_INCLUDE_DIR})
target_link_libraries(voidscript-module-${MODULE_NAME} PRIVATE voidscript ${SQLITE3_LIB})

install(TARGETS voidscript-module-${MODULE_NAME}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules
        COMPONENT "modules-${MODULE_NAME}")
