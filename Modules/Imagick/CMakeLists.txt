# Modules/IMAGICK/CMakeLists.txt

if (NOT BUILD_MODULE_IMAGICK)
    return()
endif()

project(
    modules-imagick
    VERSION 0.0.1
    DESCRIPTION "A simple ImageMagick module for voidscript"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)

add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )

find_package(ImageMagick REQUIRED COMPONENTS Magick++)

add_library(voidscript-module-imagick SHARED)
target_sources(voidscript-module-imagick
    PRIVATE
        src/ImagickModule.cpp
        src/PluginInit.cpp
)

target_include_directories(voidscript-module-imagick PRIVATE ${ImageMagick_INCLUDE_DIRS})
target_link_libraries(voidscript-module-imagick PRIVATE ${ImageMagick_LIBRARIES})

install(TARGETS voidscript-module-imagick DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules COMPONENT "modules-imagick")
