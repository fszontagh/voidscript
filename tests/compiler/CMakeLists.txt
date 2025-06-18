# Compiler-specific tests CMakeLists.txt

if(BUILD_COMPILER)
    # Add compiler test executable
    add_executable(compiler_tests
        compiler_basic_tests.cpp
    )
    
    # Link against the main voidscript library and Catch2
    target_link_libraries(compiler_tests PRIVATE voidscript Catch2::Catch2WithMain)
    
    # Discover and register tests
    catch_discover_tests(compiler_tests)
    
    # Add additional compiler-specific tests here
    message(STATUS "Compiler tests configured")
else()
    message(STATUS "Compiler tests skipped - BUILD_COMPILER is OFF")
endif()