if (NOT BUILD_MODULE_MEMCACHED)
    return()
endif()

project(
    modules-memcached
    VERSION 0.0.1
    DESCRIPTION "Memcached support module for VoidScript"
    LANGUAGES CXX
)

# Find libmemcached-dev
find_path(MEMCACHED_INCLUDE_DIR NAMES libmemcached/memcached.h)
find_library(MEMCACHED_LIB NAMES memcached)

if (NOT MEMCACHED_INCLUDE_DIR OR NOT MEMCACHED_LIB)
    message(FATAL "libmemcached-dev not found")
endif()

    add_library(voidscript-module-memcached SHARED)
    target_sources(voidscript-module-memcached
        PRIVATE
            src/MemcachedModule.cpp
            src/PluginInit.cpp
    )
    target_include_directories(voidscript-module-memcached PRIVATE ${MEMCACHED_INCLUDE_DIR})
    target_link_libraries(voidscript-module-memcached PRIVATE voidscript ${MEMCACHED_LIB})


install(TARGETS voidscript-module-memcached DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT modules-memcached)