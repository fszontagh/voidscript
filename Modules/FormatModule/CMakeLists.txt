project(
    FormatModule
    VERSION 0.0.1
    DESCRIPTION "A simple curl module for voidscript"
    LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)

FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 11.1.4
)
FetchContent_MakeAvailable(fmt)



add_library(FormatModule SHARED)
target_sources(FormatModule
    PRIVATE
        src/FormatModule.cpp
        src/PluginInit.cpp
)

target_link_libraries(FormatModule PRIVATE fmt::fmt)