# VoidScript Compiler Test Suite

This directory contains comprehensive tests for the VoidScript compiler to verify that compiled binaries work correctly and produce expected output.

## Directory Structure

```
tests/compiler/
├── CMakeLists.txt              # CMake integration for compiler tests
├── README.md                   # This documentation
├── run_compiler_tests.sh       # Main test runner script
├── test_scripts/               # VoidScript test files to compile
│   ├── 01_basic_variables.vs   # Basic variable declarations and assignments
│   ├── 02_basic_functions.vs   # Function definitions and calls
│   ├── 03_control_flow.vs      # If statements, loops, control flow
│   ├── 04_arrays.vs            # Array operations and iteration
│   ├── 05_classes.vs           # Class definitions and method calls
│   ├── 06_objects.vs           # Object operations and property access
│   ├── 07_constants.vs         # Constants and immutable values
│   ├── 08_syntax_error.vs      # Syntax errors (should fail compilation)
│   ├── 09_type_error.vs        # Type errors (should fail compilation)
│   └── 10_comprehensive.vs     # Comprehensive test combining features
└── expected_outputs/           # Expected output files for verification
    ├── 01_basic_variables.txt
    ├── 02_basic_functions.txt
    ├── 03_control_flow.txt
    ├── 04_arrays.txt
    ├── 05_classes.txt
    ├── 06_objects.txt
    ├── 07_constants.txt
    └── 10_comprehensive.txt
```

## Test Categories

### 1. Basic Functionality Tests (01-07)
These tests verify core language features:
- **Variables**: Declaration, assignment, type handling
- **Functions**: Definition, calls, parameters, return values
- **Control Flow**: If statements, while loops, for loops
- **Arrays**: Creation, indexing, iteration
- **Classes**: Definition, instantiation, method calls
- **Objects**: Property access, iteration
- **Constants**: Immutable values, usage in expressions

### 2. Error Handling Tests (08-09)
These tests verify the compiler handles errors gracefully:
- **Syntax Errors**: Invalid syntax should fail compilation
- **Type Errors**: Type mismatches should be detected

### 3. Comprehensive Tests (10+)
These tests combine multiple features in realistic scenarios to verify:
- Complex program compilation and execution
- Integration between different language features
- Performance with larger programs

### 4. Compatibility Tests
These tests verify that compiled binaries produce the same output as the interpreter:
- Same VoidScript code run through both interpreter and compiler
- Output comparison to ensure semantic compatibility

## Running the Tests

### Method 1: Using the Shell Script (Recommended)
```bash
# From the project root
./tests/compiler/run_compiler_tests.sh

# Or from the build directory
cd build
../tests/compiler/run_compiler_tests.sh
```

### Method 2: Using CMake/CTest
```bash
# From build directory
cd build

# Run all compiler tests
ctest -L compiler

# Run specific test categories
ctest -L "compiler;compilation"     # Only compilation tests
ctest -L "compiler;execution"       # Only execution tests
ctest -L "compiler;verification"    # Only output verification tests
ctest -L "compiler;error_handling"  # Only error handling tests
ctest -L "compiler;compatibility"   # Only compatibility tests

# Run the full test suite
ctest -R "compiler_full_test_suite"

# Run with verbose output
ctest -L compiler --output-on-failure
```

### Method 3: Using Make Target
```bash
# From build directory
make test_compiler
```

### Method 4: Manual Testing
```bash
# Compile a single test
./build/compiler/voidscript-compiler tests/compiler/test_scripts/01_basic_variables.vs

# Run the compiled binary
./01_basic_variables

# Compare with expected output
diff tests/compiler/expected_outputs/01_basic_variables.txt <(./01_basic_variables)
```

## Test Features

### Automatic Cleanup
- Generated binaries are automatically cleaned up after tests
- Intermediate files are removed unless `--keep-intermediate` is used
- Temporary test directories are cleaned on exit

### Timeout Handling
- All tests have reasonable timeouts to prevent hanging
- Default timeout: 30 seconds for compilation and execution
- Full test suite timeout: 5 minutes

### Cross-Platform Compatibility
- Tests are designed to work on Linux (primary focus)
- Shell script uses portable bash features
- CMake integration works across platforms

### Memory Leak Detection
- Test infrastructure can be extended for memory leak detection
- Valgrind integration can be added for memory analysis

## Test Infrastructure Features

### Output Verification
- Each test script has corresponding expected output file
- Byte-for-byte comparison of actual vs expected output
- Diff output shown for failed comparisons

### Compilation Verification
- Tests verify successful compilation before execution
- Compilation errors are captured and displayed
- Binary existence is verified after compilation

### Error Testing
- Invalid scripts are tested to ensure they fail compilation
- Error messages are captured for analysis
- Graceful error handling verification

### Compatibility Testing
- Same scripts run through both interpreter and compiler
- Output comparison ensures semantic compatibility
- Automatic detection of available interpreter

## Adding New Tests

### 1. Create Test Script
Add a new `.vs` file in `test_scripts/` following the naming convention:
- `XX_test_name.vs` where XX is a number for ordering
- Use descriptive names that indicate what is being tested

### 2. Create Expected Output (Optional)
If your test produces output, create corresponding `.txt` file in `expected_outputs/`:
- Same base name as test script
- Contains exact expected output including newlines

### 3. Categorize the Test
Tests are automatically categorized based on filename patterns:
- `01-07`: Basic functionality tests (with compatibility testing)
- `08-09`: Error tests (should fail compilation)
- `10+`: Comprehensive tests (with compatibility testing)

### 4. Update Documentation
Update this README if adding new test categories or significant features.

## Troubleshooting

### Common Issues

1. **Compiler not found**
   ```
   Error: Compiler not found at: build/compiler/voidscript-compiler
   ```
   - Solution: Build the compiler first: `cd build && ninja voidscript-compiler`

2. **Permission denied on test script**
   ```
   Error: Permission denied: ./tests/compiler/run_compiler_tests.sh
   ```
   - Solution: Make script executable: `chmod +x tests/compiler/run_compiler_tests.sh`

3. **Output mismatch**
   ```
   Error: Output mismatch for test_name
   ```
   - Check the diff output for differences
   - Verify expected output file is correct
   - Consider platform-specific output differences

4. **Tests timing out**
   ```
   Error: Test timed out after 30 seconds
   ```
   - Increase timeout in test configuration
   - Check for infinite loops in test scripts
   - Verify compiler is not hanging

### Debug Mode
Run tests with verbose output to see detailed information:
```bash
# Shell script with debug info
VERBOSE=1 ./tests/compiler/run_compiler_tests.sh

# CMake with verbose output
ctest -L compiler --verbose --output-on-failure
```

## Integration with CI/CD

The test suite is designed for integration with continuous integration systems:

- Exit codes: 0 for success, non-zero for failure
- Structured output for parsing by CI tools
- Reasonable timeouts to prevent CI hanging
- Cleanup on both success and failure
- Multiple output formats (shell script and CMake)

## Performance Considerations

- Tests are designed to run quickly (most complete in seconds)
- Parallel execution is supported through CMake
- Temporary files are created in dedicated directories
- Resource usage is minimized

## Future Enhancements

Potential improvements to the test suite:

1. **Memory Analysis**: Integration with Valgrind for memory leak detection
2. **Performance Testing**: Benchmarks comparing interpreter vs compiler performance
3. **Stress Testing**: Large programs and edge cases
4. **Regression Testing**: Automated detection of performance regressions
5. **Code Coverage**: Analysis of compiler code coverage during tests
6. **Multi-Platform Testing**: Extended support for Windows and macOS
7. **Fuzzing**: Automated generation of test cases for robustness testing

## Contributing

When contributing to the compiler test suite:

1. Follow existing naming conventions
2. Include both positive and negative test cases
3. Provide clear expected outputs
4. Document any special requirements
5. Test on multiple platforms when possible
6. Update this documentation for significant changes

## License

These tests are part of the VoidScript project and are subject to the same license terms as the main project.