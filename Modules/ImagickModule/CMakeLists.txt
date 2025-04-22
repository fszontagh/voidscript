project(
    ImagickModule
    VERSION 0.0.1
    DESCRIPTION "A simple curl module for voidscript"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)

add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )
find_package(ImageMagick REQUIRED COMPONENTS Magick++)


add_library(ImagickModule SHARED)
target_sources(ImagickModule
    PRIVATE
        src/ImagickModule.cpp
        src/PluginInit.cpp
)

include_directories(${ImageMagick_INCLUDE_DIRS})
target_link_libraries(ImagickModule ${ImageMagick_LIBRARIES})