cmake_minimum_required(VERSION 3.20)
project(
    voidscript
    LANGUAGES CXX
    VERSION 0.0.1
    DESCRIPTION "A simple scripting language"
    HOMEPAGE_URL "https://github.com/fszontagh/voidshell"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(NEED_CLI ON)

option(BUILD_CLI, "Build example commandline intrepeter" ${NEED_CLI})
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)


if (BUILD_CLI)
    set(NEED_CLI ${BUILD_CLI})
endif()

include(cmake/AppVersion.cmake)

set(IDENTIFIER_COMMENT "#")
set(PARSER_OPEN_TAG "<?void")
set(PARSER_CLOSE_TAG "?>")
set(IDENTIFIER_FUNCTION "function")
set(IDENTIFIER_VARIABLE "$")
set(IDENTIFIER_IF "if")
set(IDENTIFIER_RETURN "return")

message(STATUS "BUILD_CLI:             ${NEED_CLI}")
message(STATUS "BUILD_SHARED_LIBS:     ${BUILD_SHARED_LIBS}")
message(STATUS "  IDENTIFIER_COMMENT:  ${IDENTIFIER_COMMENT}")
message(STATUS "  IDENTIFIER_FUNCTION: ${IDENTIFIER_FUNCTION}")
message(STATUS "  IDENTIFIER_VARIABLE: ${IDENTIFIER_VARIABLE}")
message(STATUS "  IDENTIFIER_IF:       ${IDENTIFIER_IF}")
message(STATUS "  IDENTIFIER_RETURN:   ${IDENTIFIER_RETURN}")
message(STATUS "  PARSER_OPEN_TAG:     ${PARSER_OPEN_TAG}")
message(STATUS "  PARSER_CLOSE_TAG:    ${PARSER_CLOSE_TAG}")
message(STATUS "APP_GIT_VERSION:       ${APP_GIT_VERSION}")

if (CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    set(DEBUG_BUILD ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# PACKAGING PROPERTIES
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/assets/linux/DESCRIPTION.txt")
set(CPACK_PACKAGE_CONTACT "Ferenc Szontágh <szf@fsociety.hu>")
set(CPACK_PACKAGE_VENDOR "fszontagh")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/fszontagh/voidscript")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Voidscript a simple scripting language")
set(CPACK_PACKAGE_CHECKSUM "SHA512")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
#set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_STRIP_FILES YES)

set(
    CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)
set(CPACK_PACKAGING_INSTALL_PREFIX "/")

set(ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    execute_process(
        COMMAND bash -c "source /etc/lsb-release && echo \"$DISTRIB_CODENAME\""
        OUTPUT_VARIABLE DISTRIB_CODENAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(CPACK_SYSTEM_NAME ${DISTRIB_CODENAME})
    message(STATUS "Detected DISTRIB_CODENAME: ${DISTRIB_CODENAME}")

    find_program(DPKG dpkg)
    if (DPKG)
        execute_process(COMMAND ${DPKG} --print-architecture OUTPUT_VARIABLE ARCHITECTURE OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()


    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCHITECTURE "amd64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(ARCHITECTURE "i386")
    endif()
endif()

message(STATUS "System architecture: ${ARCHITECTURE}")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "/")
endif()

include(GNUInstallDirs)
# PACKAGING PROPERTIES END

configure_file("cmake/options.h.in" "include/options.h" @ONLY)
configure_file("test_scripts/test1.vs" "test_scripts/test1.vs" @ONLY)
configure_file("test_scripts/test2.vs" "test_scripts/test2.vs" @ONLY)

include_directories(${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR}/src)


# LIBRARY TARGET
add_library(voidscript
            src/Parser/Parser.cpp
            src/Lexer/Lexer.cpp
)

install(TARGETS voidscript DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT "lib")
set(CPACK_DEBIAN_LIB_FILE_NAME "libvoidscript_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.deb")
set(CPACK_DEBIAN_LIB_PACKAGE_NAME "libvoidscript")
set(CPACK_DEBIAN_LIB_PACKAGE_SECTION "libs")
set_target_properties(voidscript PROPERTIES VERSION ${CMAKE_PROJECT_VERSION} SOVERSION 1)
message(STATUS "CMAKE_SYSTEM: ${CMAKE_SYSTEM}")

set_target_properties(voidscript PROPERTIES
    LINKER_LANGUAGE CXX
    LIBRARY_OUTPUT_NAME voidscript
)


# EXECUTABLE TARGET
if (NEED_CLI)
    add_executable(voidscript-cli cli/main.cpp)
    target_link_libraries(voidscript-cli voidscript)
    install(TARGETS voidscript-cli DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT "bin")
    set_target_properties(voidscript-cli PROPERTIES
    LINKER_LANGUAGE CXX
    OUTPUT_NAME voidscript)
    set(CPACK_PACKAGE_EXECUTABLES voidscript-cli "Simple Shell")
    set(CPACK_DEBIAN_BIN_FILE_NAME "voidscript_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.deb")
    set(CPACK_DEBIAN_BIN_PACKAGE_NAME "voidscript")
    set(CPACK_DEBIAN_BIN_PACKAGE_SECTION "interpreters")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS ${CMAKE_BINARY_DIR})
    set(CPACK_DEBIAN_BIN_PACKAGE_DEPENDS "libvoidscript (= ${CMAKE_PROJECT_VERSION})")
endif()

# CPACK CONFIGURATION
set(CPACK_DEB_COMPONENT_INSTALL ON)

include(CPack)