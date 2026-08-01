# Modules/Exif/CMakeLists.txt - read and edit EXIF metadata via exiv2.
if (NOT BUILD_MODULE_EXIF)
    return()
endif()
include(module.cmake)
project(modules-${MODULE_NAME} VERSION 0.0.1 DESCRIPTION "${MODULE_DESCRIPTION}"
        LANGUAGES CXX HOMEPAGE_URL "https://github.com/fszontagh/voidshell")

find_package(exiv2 CONFIG QUIET)
if (NOT exiv2_FOUND)
    message(WARNING "Exif module: exiv2 (libexiv2-dev) not found - skipping module")
    return()
endif()

add_library(voidscript-module-${MODULE_NAME} SHARED)
target_sources(voidscript-module-${MODULE_NAME} PRIVATE src/ExifModule.cpp src/PluginInit.cpp)
target_link_libraries(voidscript-module-${MODULE_NAME} PRIVATE voidscript Exiv2::exiv2lib)
install(TARGETS voidscript-module-${MODULE_NAME}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT "modules-${MODULE_NAME}")
