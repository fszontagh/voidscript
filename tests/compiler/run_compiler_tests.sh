#!/bin/bash

# VoidScript Compiler Test Runner
# Compiles VoidScript test files and verifies output matches expected results

set -e  # Exit on error

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
COMPILER="${BUILD_DIR}/compiler/voidscript-compiler"
INTERPRETER="${BUILD_DIR}/voidscript"
TEST_SCRIPTS_DIR="${SCRIPT_DIR}/test_scripts"
EXPECTED_OUTPUTS_DIR="${SCRIPT_DIR}/expected_outputs"
TEMP_DIR="${SCRIPT_DIR}/temp"
TIMEOUT=30

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
ERROR_TESTS=0

# Helper functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $1"
}

cleanup() {
    if [ -d "${TEMP_DIR}" ]; then
        rm -rf "${TEMP_DIR}"
    fi
}

# Setup
setup() {
    log_info "Setting up compiler tests..."
    
    # Check if compiler exists
    if [ ! -f "${COMPILER}" ]; then
        log_error "Compiler not found at: ${COMPILER}"
        log_info "Please build the project first: cd ${BUILD_DIR} && ninja voidscript-compiler"
        exit 1
    fi
    
    # Check if interpreter exists (for compatibility tests)
    if [ ! -f "${INTERPRETER}" ]; then
        log_warning "Interpreter not found at: ${INTERPRETER}"
        log_info "Compatibility tests will be skipped"
    fi
    
    # Create temp directory
    mkdir -p "${TEMP_DIR}"
    
    log_info "Compiler: ${COMPILER}"
    log_info "Test scripts: ${TEST_SCRIPTS_DIR}"
    log_info "Expected outputs: ${EXPECTED_OUTPUTS_DIR}"
    log_info "Temp directory: ${TEMP_DIR}"
    echo
}

# Run a single compilation test
run_compilation_test() {
    local test_file="$1"
    local test_name="$(basename "${test_file}" .vs)"
    local binary="${TEMP_DIR}/${test_name}"
    local output_file="${TEMP_DIR}/${test_name}_output.txt"
    local expected_file="${EXPECTED_OUTPUTS_DIR}/${test_name}.txt"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    log_info "Testing: ${test_name}"
    
    # Step 1: Compile the script
    if ! timeout ${TIMEOUT} "${COMPILER}" --output "${binary}" "${test_file}" > "${TEMP_DIR}/${test_name}_compile.log" 2>&1; then
        log_error "Compilation failed for ${test_name}"
        cat "${TEMP_DIR}/${test_name}_compile.log"
        ERROR_TESTS=$((ERROR_TESTS + 1))
        return 1
    fi
    
    # Step 2: Check if binary was created
    if [ ! -f "${binary}" ]; then
        log_error "Binary not created for ${test_name}"
        ERROR_TESTS=$((ERROR_TESTS + 1))
        return 1
    fi
    
    # Step 3: Execute the binary
    if ! timeout ${TIMEOUT} "${binary}" > "${output_file}" 2>&1; then
        log_error "Execution failed for ${test_name}"
        if [ -f "${output_file}" ]; then
            cat "${output_file}"
        fi
        ERROR_TESTS=$((ERROR_TESTS + 1))
        return 1
    fi
    
    # Step 4: Compare with expected output (if exists)
    if [ -f "${expected_file}" ]; then
        if diff -u "${expected_file}" "${output_file}" > "${TEMP_DIR}/${test_name}_diff.txt"; then
            log_success "Output matches expected for ${test_name}"
            PASSED_TESTS=$((PASSED_TESTS + 1))
            return 0
        else
            log_error "Output mismatch for ${test_name}"
            echo "Expected vs Actual:"
            cat "${TEMP_DIR}/${test_name}_diff.txt"
            FAILED_TESTS=$((FAILED_TESTS + 1))
            return 1
        fi
    else
        log_warning "No expected output file for ${test_name}, test passed (compilation only)"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    fi
}

# Run error tests (should fail compilation)
run_error_test() {
    local test_file="$1"
    local test_name="$(basename "${test_file}" .vs)"
    local binary="${TEMP_DIR}/${test_name}"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    log_info "Testing error case: ${test_name}"
    
    # This should fail
    if timeout ${TIMEOUT} "${COMPILER}" --output "${binary}" "${test_file}" > "${TEMP_DIR}/${test_name}_compile.log" 2>&1; then
        log_error "Error test ${test_name} should have failed compilation but succeeded"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    else
        log_success "Error test ${test_name} correctly failed compilation"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    fi
}

# Run compatibility test (compare interpreter vs compiler output)
run_compatibility_test() {
    local test_file="$1"
    local test_name="$(basename "${test_file}" .vs)"
    local binary="${TEMP_DIR}/${test_name}"
    local compiler_output="${TEMP_DIR}/${test_name}_compiler.txt"
    local interpreter_output="${TEMP_DIR}/${test_name}_interpreter.txt"
    
    if [ ! -f "${INTERPRETER}" ]; then
        log_warning "Skipping compatibility test for ${test_name} (interpreter not available)"
        return 0
    fi
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    log_info "Testing compatibility: ${test_name}"
    
    # Compile and run with compiler
    if ! timeout ${TIMEOUT} "${COMPILER}" --output "${binary}" "${test_file}" > /dev/null 2>&1; then
        log_warning "Skipping compatibility test for ${test_name} (compilation failed)"
        return 0
    fi
    
    if ! timeout ${TIMEOUT} "${binary}" > "${compiler_output}" 2>&1; then
        log_warning "Skipping compatibility test for ${test_name} (execution failed)"
        return 0
    fi
    
    # Run with interpreter
    if ! timeout ${TIMEOUT} "${INTERPRETER}" "${test_file}" > "${interpreter_output}" 2>&1; then
        log_warning "Skipping compatibility test for ${test_name} (interpreter failed)"
        return 0
    fi
    
    # Compare outputs
    if diff -u "${interpreter_output}" "${compiler_output}" > "${TEMP_DIR}/${test_name}_compat_diff.txt"; then
        log_success "Compatibility test passed for ${test_name}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        log_error "Compatibility test failed for ${test_name}"
        echo "Interpreter vs Compiler output:"
        cat "${TEMP_DIR}/${test_name}_compat_diff.txt"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
}

# Main test execution
run_tests() {
    log_info "Running VoidScript compiler tests..."
    echo
    
    # Basic functionality tests
    echo "=== Basic Functionality Tests ==="
    for test_file in "${TEST_SCRIPTS_DIR}"/0[1-7]_*.vs; do
        if [ -f "${test_file}" ]; then
            run_compilation_test "${test_file}"
            echo
        fi
    done
    
    # Error handling tests
    echo "=== Error Handling Tests ==="
    for test_file in "${TEST_SCRIPTS_DIR}"/0[8-9]_*.vs; do
        if [ -f "${test_file}" ]; then
            run_error_test "${test_file}"
            echo
        fi
    done
    
    # Comprehensive tests
    echo "=== Comprehensive Tests ==="
    for test_file in "${TEST_SCRIPTS_DIR}"/10_*.vs; do
        if [ -f "${test_file}" ]; then
            run_compilation_test "${test_file}"
            echo
        fi
    done
    
    # Compatibility tests
    echo "=== Compatibility Tests ==="
    for test_file in "${TEST_SCRIPTS_DIR}"/0[1-7]_*.vs "${TEST_SCRIPTS_DIR}"/10_*.vs; do
        if [ -f "${test_file}" ]; then
            run_compatibility_test "${test_file}"
            echo
        fi
    done
}

# Print summary
print_summary() {
    echo "======================================="
    echo "VoidScript Compiler Test Summary"
    echo "======================================="
    echo "Total tests:  ${TOTAL_TESTS}"
    echo "Passed:       ${PASSED_TESTS}"
    echo "Failed:       ${FAILED_TESTS}"
    echo "Errors:       ${ERROR_TESTS}"
    echo "======================================="
    
    if [ ${FAILED_TESTS} -eq 0 ] && [ ${ERROR_TESTS} -eq 0 ]; then
        log_success "All tests passed!"
        return 0
    else
        log_error "Some tests failed!"
        return 1
    fi
}

# Signal handlers
trap cleanup EXIT
trap 'echo; log_error "Test interrupted!"; exit 130' INT TERM

# Main execution
main() {
    setup
    run_tests
    print_summary
}

# Run if called directly
if [ "${BASH_SOURCE[0]}" == "${0}" ]; then
    main "$@"
fi