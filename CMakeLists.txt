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






option(BUILD_FASTCGI "Enable building FastCGI executable" ON)

option(BUILD_TESTS "Build the test cases" OFF)



if (BUILD_CLI)
    set(NEED_CLI ${BUILD_CLI})
endif()

include(cmake/AppVersion.cmake)

set(IDENTIFIER_COMMENT "#")
set(PARSER_OPEN_TAG "<?void")
set(PARSER_CLOSE_TAG "?>")


if (CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()



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

set(MODULES_FOLDER "${CMAKE_INSTALL_FULL_DATADIR}/${CMAKE_PROJECT_NAME}/Modules" CACHE PATH "OS specific modules folder")

message(STATUS "CMAKE_INSTALL_FULL_DATADIR ${CMAKE_INSTALL_FULL_DATADIR}")

# PACKAGING PROPERTIES END

configure_file("cmake/options.h.in" "include/options.h" @ONLY)
    configure_file("test_scripts/variables.vs" "test_scripts/variables.vs" @ONLY)
    configure_file("test_scripts/expressions.vs" "test_scripts/expressions.vs" @ONLY)




include_directories(${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR}/src)


# LIBRARY TARGET
add_library(voidscript
            src/Parser/Parser.cpp
            src/Lexer/Lexer.cpp
            src/Lexer/Operators.cpp
            src/Symbols/ClassRegistry.cpp
            src/Modules/BuiltIn/ModuleHelperModule.cpp
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
    # Mark this build as CLI variant
    target_compile_definitions(voidscript-cli PRIVATE CLI)
    target_link_libraries(voidscript-cli voidscript)
    install(TARGETS voidscript-cli DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT "bin")
    set_target_properties(voidscript-cli PROPERTIES
        LINKER_LANGUAGE CXX
        OUTPUT_NAME voidscript)

    set(CPACK_PACKAGE_EXECUTABLES voidscript-cli "Simple Shell")

    set(CPACK_DEBIAN_BIN_FILE_NAME "${CMAKE_PROJECT_NAME}_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.deb")
    set(CPACK_DEBIAN_BIN_PACKAGE_NAME "voidscript")
    set(CPACK_DEBIAN_BIN_PACKAGE_SECTION "interpreters")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS ${CMAKE_BINARY_DIR})
    set(CPACK_DEBIAN_BIN_PACKAGE_DEPENDS "libvoidscript (= ${CMAKE_PROJECT_VERSION})")

    set(CPACK_RPM_BIN_FILE_NAME "${CMAKE_PROJECT_NAME}_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.rpm")
    set(CPACK_RPM_BIN_PACKAGE_NAME "voidscript")
    set(CPACK_RPM_BIN_PACKAGE_REQUIRES "libvoidscript = ${CMAKE_PROJECT_VERSION}")
endif()


if (BUILD_FASTCGI)
    add_subdirectory(fastcgi)

    set(CPACK_DEBIAN_FCGI_FILE_NAME "${CMAKE_PROJECT_NAME}-fcgi_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.deb")
    set(CPACK_DEBIAN_FCGI_PACKAGE_NAME "${CMAKE_PROJECT_NAME}-fcgi")
    set(CPACK_DEBIAN_FCGI_PACKAGE_SECTION "web")
    set(CPACK_DEBIAN_FCGI_PACKAGE_DEPENDS "libvoidscript (= ${CMAKE_PROJECT_VERSION})")

    set(CPACK_RPM_FCGI_FILE_NAME "${CMAKE_PROJECT_NAME}-fcgi_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.rpm")
    set(CPACK_RPM_FCGI_PACKAGE_NAME "${CMAKE_PROJECT_NAME}-fcgi")
    set(CPACK_RPM_FCGI_PACKAGE_GROUP "Applications/Web")
    set(CPACK_RPM_FCGI_PACKAGE_REQUIRES "libvoidscript = ${CMAKE_PROJECT_VERSION}")
endif()


## MODULES

macro(add_dynamic_module MODULE_NAME)
    string(TOUPPER "${MODULE_NAME}" UPPER_MODULE_NAME)
    string(TOLOWER "${MODULE_NAME}" LOWER_MODULE_NAME)
    set(opt_name "BUILD_MODULE_${UPPER_MODULE_NAME}")
    option(${opt_name} "Enable ${UPPER_MODULE_NAME} module" ON)

    if(${opt_name})
        message(STATUS "Adding module: ${MODULE_NAME}")
        add_subdirectory(Modules/${MODULE_NAME})

        string(CONCAT debian_var_name "CPACK_DEBIAN_" "MODULES-${UPPER_MODULE_NAME}" "_FILE_NAME")
        string(CONCAT rpm_var_name "CPACK_RPM_" "MODULES-${UPPER_MODULE_NAME}" "_FILE_NAME")

        set(${debian_var_name} "${CMAKE_PROJECT_NAME}-modules-${LOWER_MODULE_NAME}_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.deb")

        set(${rpm_var_name}    "${CMAKE_PROJECT_NAME}-modules-${LOWER_MODULE_NAME}_${CMAKE_PROJECT_VERSION}_${ARCHITECTURE}_${CPACK_SYSTEM_NAME}.rpm")
    endif()
endmacro()


file(GLOB AVAILABLE_MODULES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/Modules
    "Modules/*"
)


message(STATUS "APP_GIT_VERSION:       ${APP_GIT_VERSION}")
message(STATUS "APP_VERSION:           ${CMAKE_PROJECT_VERSION}\n")
message(STATUS "BUILD_CLI:             ${NEED_CLI}")
message(STATUS "BUILD_FASTCGI:         ${BUILD_FASTCGI}")
message(STATUS "BUILD_SHARED_LIBS:     ${BUILD_SHARED_LIBS}")
message(STATUS "BUILD_TESTS:           ${BUILD_TESTS}\n")
message(STATUS "  PARSER_OPEN_TAG:     '${PARSER_OPEN_TAG}'")
message(STATUS "  PARSER_CLOSE_TAG:    '${PARSER_CLOSE_TAG}'\n")


message(STATUS "  MODULES_FOLDER:            ${MODULES_FOLDER}")
foreach(MODULE IN LISTS AVAILABLE_MODULES)
    add_dynamic_module(${MODULE})
endforeach(MODULE AVAILABLE_MODULES)

# CPACK CONFIGURATION
set(CPACK_DEB_COMPONENT_INSTALL ON)


if (BUILD_TESTS)
  enable_testing()
  include(cmake/catch2.cmake)

  # Test executable for ModuleHelperModule
  add_executable(module_helper_tests
      tests/ModuleHelperTests.cpp
  )
  target_link_libraries(module_helper_tests PRIVATE voidscript Catch2::Catch2WithMain)
  # Discover and register tests with CTest
  include(CTest)
  include(Catch)
  catch_discover_tests(module_helper_tests)
  # Test executable for all built-in modules
  add_executable(built_in_module_tests
      tests/BuiltInModuleTests.cpp
  )
  target_link_libraries(built_in_module_tests PRIVATE voidscript Catch2::Catch2WithMain)
  catch_discover_tests(built_in_module_tests)
endif()


include(CPack)
