# VoidScript Compiler Tests CMake Configuration

# Ensure the compiler is built before running tests
if(NOT TARGET voidscript-compiler)
    message(FATAL_ERROR "voidscript-compiler target not found. Compiler tests require the compiler to be built.")
endif()

# Find test script files
file(GLOB COMPILER_TEST_SCRIPTS
    "${CMAKE_CURRENT_SOURCE_DIR}/test_scripts/*.vs"
)

# Helper function to add compiler tests
function(add_compiler_test TEST_NAME SCRIPT_FILE)
    set(BINARY_NAME "${TEST_NAME}_compiled")
    set(BINARY_PATH "${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}")
    
    # Test compilation
    add_test(
        NAME "compiler_compile_${TEST_NAME}"
        COMMAND voidscript-compiler --output "${BINARY_PATH}" "${SCRIPT_FILE}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
    set_tests_properties("compiler_compile_${TEST_NAME}" PROPERTIES
        TIMEOUT 30
        LABELS "compiler;compilation"
    )
    
    # Test execution (depends on successful compilation)
    add_test(
        NAME "compiler_execute_${TEST_NAME}"
        COMMAND "${BINARY_PATH}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
    set_tests_properties("compiler_execute_${TEST_NAME}" PROPERTIES
        TIMEOUT 30
        LABELS "compiler;execution"
        DEPENDS "compiler_compile_${TEST_NAME}"
    )
    
    # Output verification test (if expected output exists)
    set(EXPECTED_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/expected_outputs/${TEST_NAME}.txt")
    if(EXISTS "${EXPECTED_OUTPUT}")
        set(ACTUAL_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_output.txt")
        
        add_test(
            NAME "compiler_verify_${TEST_NAME}"
            COMMAND "${CMAKE_COMMAND}" -E compare_files "${EXPECTED_OUTPUT}" "${ACTUAL_OUTPUT}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
        set_tests_properties("compiler_verify_${TEST_NAME}" PROPERTIES
            TIMEOUT 10
            LABELS "compiler;verification"
            DEPENDS "compiler_execute_${TEST_NAME}"
        )
        
        # Create a test that captures output for verification
        add_test(
            NAME "compiler_capture_${TEST_NAME}"
            COMMAND "${CMAKE_COMMAND}" -E env "${BINARY_PATH}" > "${ACTUAL_OUTPUT}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
        set_tests_properties("compiler_capture_${TEST_NAME}" PROPERTIES
            TIMEOUT 30
            LABELS "compiler;capture"
            DEPENDS "compiler_compile_${TEST_NAME}"
        )
    endif()
endfunction()

# Helper function to add error tests (should fail compilation)
function(add_compiler_error_test TEST_NAME SCRIPT_FILE)
    set(BINARY_NAME "${TEST_NAME}_should_fail")
    set(BINARY_PATH "${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}")
    
    add_test(
        NAME "compiler_error_${TEST_NAME}"
        COMMAND voidscript-compiler --output "${BINARY_PATH}" "${SCRIPT_FILE}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
    set_tests_properties("compiler_error_${TEST_NAME}" PROPERTIES
        TIMEOUT 30
        LABELS "compiler;error_handling"
        WILL_FAIL TRUE  # This test should fail
    )
endfunction()

# Helper function to add compatibility tests (interpreter vs compiler)
function(add_compiler_compatibility_test TEST_NAME SCRIPT_FILE)
    if(TARGET voidscript-cli)
        set(BINARY_NAME "${TEST_NAME}_compat")
        set(BINARY_PATH "${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}")
        set(COMPILER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_compiler_output.txt")
        set(INTERPRETER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_interpreter_output.txt")
        
        # Compile with compiler
        add_test(
            NAME "compiler_compat_compile_${TEST_NAME}"
            COMMAND voidscript-compiler --output "${BINARY_PATH}" "${SCRIPT_FILE}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
        
        # Run compiled binary
        add_test(
            NAME "compiler_compat_run_${TEST_NAME}"
            COMMAND "${CMAKE_COMMAND}" -E env "${BINARY_PATH}" > "${COMPILER_OUTPUT}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
        
        # Run with interpreter
        add_test(
            NAME "interpreter_compat_run_${TEST_NAME}"
            COMMAND "${CMAKE_COMMAND}" -E env voidscript-cli "${SCRIPT_FILE}" > "${INTERPRETER_OUTPUT}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
        
        # Compare outputs
        add_test(
            NAME "compiler_compat_verify_${TEST_NAME}"
            COMMAND "${CMAKE_COMMAND}" -E compare_files "${INTERPRETER_OUTPUT}" "${COMPILER_OUTPUT}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
        
        set_tests_properties(
            "compiler_compat_compile_${TEST_NAME}"
            "compiler_compat_run_${TEST_NAME}"
            "interpreter_compat_run_${TEST_NAME}"
            "compiler_compat_verify_${TEST_NAME}"
            PROPERTIES
            TIMEOUT 30
            LABELS "compiler;compatibility"
        )
        
        # Set dependencies
        set_tests_properties("compiler_compat_run_${TEST_NAME}" PROPERTIES
            DEPENDS "compiler_compat_compile_${TEST_NAME}"
        )
        set_tests_properties("compiler_compat_verify_${TEST_NAME}" PROPERTIES
            DEPENDS "compiler_compat_run_${TEST_NAME};interpreter_compat_run_${TEST_NAME}"
        )
    endif()
endfunction()

# Add basic functionality tests
foreach(SCRIPT_FILE ${COMPILER_TEST_SCRIPTS})
    get_filename_component(SCRIPT_NAME "${SCRIPT_FILE}" NAME_WE)
    
    # Categorize tests based on filename pattern
    if(SCRIPT_NAME MATCHES "^0[1-7]_.*")
        # Basic functionality tests (01-07)
        add_compiler_test("${SCRIPT_NAME}" "${SCRIPT_FILE}")
        add_compiler_compatibility_test("${SCRIPT_NAME}" "${SCRIPT_FILE}")
    elseif(SCRIPT_NAME MATCHES "^0[8-9]_.*")
        # Error tests (08-09)
        add_compiler_error_test("${SCRIPT_NAME}" "${SCRIPT_FILE}")
    elseif(SCRIPT_NAME MATCHES "^10_.*")
        # Comprehensive tests (10+)
        add_compiler_test("${SCRIPT_NAME}" "${SCRIPT_FILE}")
        add_compiler_compatibility_test("${SCRIPT_NAME}" "${SCRIPT_FILE}")
    else
        # Other tests - treat as basic functionality
        add_compiler_test("${SCRIPT_NAME}" "${SCRIPT_FILE}")
    endif()
endforeach()

# Add a master test that runs the shell script
add_test(
    NAME "compiler_full_test_suite"
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/run_compiler_tests.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties("compiler_full_test_suite" PROPERTIES
    TIMEOUT 300  # 5 minutes for full suite
    LABELS "compiler;full_suite"
)

# Custom target to run only compiler tests
add_custom_target(test_compiler
    COMMAND ${CMAKE_CTEST_COMMAND} -L "compiler" --output-on-failure
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running VoidScript compiler tests"
    VERBATIM
)

# Make sure compiler is built before running tests
add_dependencies(test_compiler voidscript-compiler)
if(TARGET voidscript-cli)
    add_dependencies(test_compiler voidscript-cli)
endif()

# Create a convenience script for running compiler tests
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/run_compiler_tests.sh"
    "${CMAKE_CURRENT_BINARY_DIR}/run_compiler_tests.sh"
    COPYONLY
)

message(STATUS "Compiler tests configured:")
message(STATUS "  Test scripts: ${CMAKE_CURRENT_SOURCE_DIR}/test_scripts/")
message(STATUS "  Expected outputs: ${CMAKE_CURRENT_SOURCE_DIR}/expected_outputs/")
message(STATUS "  Run with: ctest -L compiler")
message(STATUS "  Or use: make test_compiler")
message(STATUS "  Or run: ${CMAKE_CURRENT_BINARY_DIR}/run_compiler_tests.sh")