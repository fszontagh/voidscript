# Modules/Exif/CMakeLists.txt - read and edit EXIF metadata via exiv2.
if (NOT BUILD_MODULE_EXIF)
    return()
endif()
include(module.cmake)
project(modules-${MODULE_NAME} VERSION 0.0.1 DESCRIPTION "${MODULE_DESCRIPTION}"
        LANGUAGES CXX HOMEPAGE_URL "https://github.com/fszontagh/voidshell")

# Locate exiv2 via pkg-config rather than its CMake config: the exported target name
# (Exiv2::exiv2lib) is not stable across exiv2 releases, but exiv2.pc is.
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
    pkg_check_modules(EXIV2 QUIET exiv2)
endif()
if (NOT EXIV2_FOUND)
    message(WARNING "Exif module: exiv2 (libexiv2-dev) not found - skipping module")
    return()
endif()

add_library(voidscript-module-${MODULE_NAME} SHARED)
target_sources(voidscript-module-${MODULE_NAME} PRIVATE src/ExifModule.cpp src/PluginInit.cpp)
target_include_directories(voidscript-module-${MODULE_NAME} PRIVATE ${EXIV2_INCLUDE_DIRS})
target_link_directories(voidscript-module-${MODULE_NAME} PRIVATE ${EXIV2_LIBRARY_DIRS})
target_link_libraries(voidscript-module-${MODULE_NAME} PRIVATE voidscript ${EXIV2_LIBRARIES})
target_compile_options(voidscript-module-${MODULE_NAME} PRIVATE ${EXIV2_CFLAGS_OTHER})

install(TARGETS voidscript-module-${MODULE_NAME}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT "modules-${MODULE_NAME}")
