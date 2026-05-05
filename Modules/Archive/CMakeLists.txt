if (NOT BUILD_MODULE_ARCHIVE)
    return()
endif()
include(module.cmake)
project(
    modules-${MODULE_NAME}
    VERSION 0.0.1
    DESCRIPTION "Archive module (libarchive)"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)

find_path(LIBARCHIVE_INCLUDE_DIR NAMES archive.h)
find_library(LIBARCHIVE_LIB NAMES archive)

if (NOT LIBARCHIVE_INCLUDE_DIR OR NOT LIBARCHIVE_LIB)
    message(WARNING "libarchive-dev not found — skipping Archive module. Install: apt-get install libarchive-dev")
    return()
endif()

add_library(voidscript-module-${MODULE_NAME} SHARED)
target_sources(voidscript-module-${MODULE_NAME}
    PRIVATE
        src/ArchiveModule.cpp
        src/PluginInit.cpp
)
target_include_directories(voidscript-module-${MODULE_NAME} PRIVATE ${LIBARCHIVE_INCLUDE_DIR})
target_link_libraries(voidscript-module-${MODULE_NAME} PRIVATE ${LIBARCHIVE_LIB})

install(TARGETS voidscript-module-${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT "modules-${MODULE_NAME}")
