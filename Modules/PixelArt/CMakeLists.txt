# Modules/PixelArt/CMakeLists.txt
#
# Wraps sdpixel2realpixelart (https://github.com/fszontagh/sdpixel2realpixelart) as the
# VoidScript class "PixelArt": turns AI "pixel-art-looking" images into real pixel art by
# detecting the underlying pixel grid, snapping to it and quantizing the palette.
#
# The tool's library sources are compiled straight into this .so. Point PIXELART_ROOT at
# a checkout of the repo (its src/ dir). Needs OpenCV (core, imgproc, imgcodecs).
#
# On by default, but skips itself cleanly if OpenCV or the sources are missing, so a
# machine without OpenCV still builds the rest of VoidScript.

if (NOT BUILD_MODULE_PIXELART)
    return()
endif()

set(PIXELART_ROOT "/data/aipixel2realpixel" CACHE PATH "Path to a sdpixel2realpixelart checkout")

if (NOT EXISTS "${PIXELART_ROOT}/src/pixelate.cpp")
    message(WARNING "PixelArt module: PIXELART_ROOT='${PIXELART_ROOT}' has no src/pixelate.cpp - skipping module")
    return()
endif()

find_package(OpenCV QUIET COMPONENTS core imgproc imgcodecs)
if (NOT OpenCV_FOUND)
    message(WARNING "PixelArt module: OpenCV (core, imgproc, imgcodecs) not found - skipping module")
    return()
endif()

project(
    modules-pixelart
    VERSION 0.0.1
    DESCRIPTION "Pixel-art fix (sdpixel2realpixelart) module for voidscript"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)

add_library(voidscript-module-pixelart SHARED)
target_sources(voidscript-module-pixelart
    PRIVATE
        src/PixelArtModule.cpp
        src/PluginInit.cpp
        # the tool's own library, minus its CLI entry point (main.cpp)
        "${PIXELART_ROOT}/src/config.cpp"
        "${PIXELART_ROOT}/src/utils.cpp"
        "${PIXELART_ROOT}/src/colors.cpp"
        "${PIXELART_ROOT}/src/mesh.cpp"
        "${PIXELART_ROOT}/src/pixelate.cpp"
)

target_include_directories(voidscript-module-pixelart PRIVATE
    "${PIXELART_ROOT}/src"
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(voidscript-module-pixelart PRIVATE ${OpenCV_LIBS})

install(TARGETS voidscript-module-pixelart
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules
        COMPONENT "modules-pixelart")
