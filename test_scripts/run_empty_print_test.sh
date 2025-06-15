#!/bin/bash

# Test runner script for empty print issue reproduction
# This script runs the test and shows actual vs expected output

echo "========================================"
echo "EMPTY PRINT TEST RUNNER"
echo "========================================"
echo ""

# Check if the VoidScript interpreter exists
if [ ! -f "../cli/voidscript" ] && [ ! -f "../build/cli/voidscript" ] && [ ! -f "../build/voidscript" ] && [ ! -f "./voidscript" ]; then
    echo "ERROR: VoidScript interpreter not found!"
    echo "Please compile the project first:"
    echo "  mkdir build && cd build"
    echo "  cmake .. && make"
    echo ""
    exit 1
fi

# Find the correct path to the interpreter
VOIDSCRIPT=""
if [ -f "../cli/voidscript" ]; then
    VOIDSCRIPT="../cli/voidscript"
elif [ -f "../build/cli/voidscript" ]; then
    VOIDSCRIPT="../build/cli/voidscript"
elif [ -f "../build/voidscript" ]; then
    VOIDSCRIPT="../build/voidscript"
elif [ -f "./voidscript" ]; then
    VOIDSCRIPT="./voidscript"
fi

echo "Using VoidScript interpreter: $VOIDSCRIPT"
echo ""

# Run the test and capture output
echo "Running empty_print_test.vs..."
echo "========================================"
echo "ACTUAL OUTPUT:"
echo "========================================"

# Run the test script and capture both stdout and stderr
$VOIDSCRIPT empty_print_test.vs 2>&1

echo ""
echo "========================================"
echo "ANALYSIS:"
echo "========================================"
echo ""
echo "Expected behaviors to check:"
echo "1. print(\"\") should produce NO output (not 'null' or quotes)"
echo "2. printnl(\"\") should produce ONLY a newline character"
echo "3. print(\"a\") + print(\"\") + print(\"b\") should show 'ab' on same line"
echo "4. printnl(\"test\") + printnl(\"\") + printnl(\"test2\") should have empty line between"
echo ""
echo "If you see 'null' or quotes in the output where empty strings should be,"
echo "then the issue is confirmed in the print/toString chain."
echo ""
echo "Common issues to look for:"
echo "- 'null' appearing instead of nothing"
echo "- Quotes being printed literally"
echo "- Extra characters where there should be none"
echo ""