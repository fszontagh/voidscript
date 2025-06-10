#!/bin/bash

# VoidScript Compiler Test Setup Verification
# Verifies that all test files and infrastructure are properly set up

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

verify_file() {
    local file="$1"
    local description="$2"
    
    if [ -f "${file}" ]; then
        log_success "${description}: ${file}"
        return 0
    else
        log_error "${description}: ${file} (MISSING)"
        return 1
    fi
}

verify_directory() {
    local dir="$1"
    local description="$2"
    
    if [ -d "${dir}" ]; then
        log_success "${description}: ${dir}"
        return 0
    else
        log_error "${description}: ${dir} (MISSING)"
        return 1
    fi
}

log_info "Verifying VoidScript compiler test setup..."
echo

# Check directories
log_info "Checking directory structure..."
verify_directory "${SCRIPT_DIR}" "Compiler test directory"
verify_directory "${SCRIPT_DIR}/test_scripts" "Test scripts directory"
verify_directory "${SCRIPT_DIR}/expected_outputs" "Expected outputs directory"
echo

# Check test scripts
log_info "Checking test scripts..."
test_scripts=(
    "01_basic_variables.vs"
    "02_basic_functions.vs"
    "03_control_flow.vs"
    "04_arrays.vs"
    "05_classes.vs"
    "06_objects.vs"
    "07_constants.vs"
    "08_syntax_error.vs"
    "09_type_error.vs"
    "10_comprehensive.vs"
)

for script in "${test_scripts[@]}"; do
    verify_file "${SCRIPT_DIR}/test_scripts/${script}" "Test script"
done
echo

# Check expected outputs
log_info "Checking expected output files..."
expected_outputs=(
    "01_basic_variables.txt"
    "02_basic_functions.txt"
    "03_control_flow.txt"
    "04_arrays.txt"
    "05_classes.txt"
    "06_objects.txt"
    "07_constants.txt"
    "10_comprehensive.txt"
)

for output in "${expected_outputs[@]}"; do
    verify_file "${SCRIPT_DIR}/expected_outputs/${output}" "Expected output"
done
echo

# Check infrastructure files
log_info "Checking infrastructure files..."
verify_file "${SCRIPT_DIR}/CMakeLists.txt" "CMake configuration"
verify_file "${SCRIPT_DIR}/run_compiler_tests.sh" "Test runner script"
verify_file "${SCRIPT_DIR}/README.md" "Documentation"
echo

# Check test runner permissions
log_info "Checking test runner permissions..."
if [ -x "${SCRIPT_DIR}/run_compiler_tests.sh" ]; then
    log_success "Test runner is executable"
else
    log_error "Test runner is not executable"
    log_info "Fix with: chmod +x ${SCRIPT_DIR}/run_compiler_tests.sh"
fi
echo

# Check main CMakeLists.txt integration
log_info "Checking CMake integration..."
if grep -q "add_subdirectory(tests/compiler)" "${PROJECT_ROOT}/CMakeLists.txt"; then
    log_success "Compiler tests integrated into main CMakeLists.txt"
else
    log_warning "Compiler tests not found in main CMakeLists.txt"
fi
echo

# Check compiler existence
log_info "Checking compiler availability..."
BUILD_DIR="${PROJECT_ROOT}/build"
COMPILER="${BUILD_DIR}/compiler/voidscript-compiler"

if [ -f "${COMPILER}" ]; then
    log_success "Compiler found: ${COMPILER}"
    
    # Test compiler version
    if "${COMPILER}" --version > /dev/null 2>&1; then
        log_success "Compiler is functional"
    else
        log_warning "Compiler exists but may not be functional"
    fi
else
    log_warning "Compiler not found: ${COMPILER}"
    log_info "Build with: cd ${BUILD_DIR} && ninja voidscript-compiler"
fi
echo

# Check interpreter availability
log_info "Checking interpreter availability..."
INTERPRETER="${BUILD_DIR}/voidscript"

if [ -f "${INTERPRETER}" ]; then
    log_success "Interpreter found: ${INTERPRETER}"
    log_success "Compatibility tests will be available"
else
    log_warning "Interpreter not found: ${INTERPRETER}"
    log_info "Compatibility tests will be skipped"
    log_info "Build with: cd ${BUILD_DIR} && ninja voidscript-cli"
fi
echo

# Summary
log_info "=== Verification Summary ==="
echo "Test setup verification complete!"
echo
echo "To run the compiler tests:"
echo "  1. Build the compiler: cd ${BUILD_DIR} && ninja voidscript-compiler"
echo "  2. Run tests: ${SCRIPT_DIR}/run_compiler_tests.sh"
echo "  3. Or use CMake: cd ${BUILD_DIR} && ctest -L compiler"
echo
echo "For detailed documentation, see:"
echo "  ${SCRIPT_DIR}/README.md"