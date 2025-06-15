#!/bin/bash

# Quick validation script for empty print issue
# Runs the minimal test and provides immediate pass/fail feedback

echo "Empty Print Issue Validator"
echo "=========================="

# Find VoidScript interpreter
VOIDSCRIPT=""
if [ -f "../build/voidscript" ]; then
    VOIDSCRIPT="../build/voidscript"
elif [ -f "../build/cli/voidscript" ]; then
    VOIDSCRIPT="../build/cli/voidscript"
elif [ -f "../cli/voidscript" ]; then
    VOIDSCRIPT="../cli/voidscript"
elif [ -f "./voidscript" ]; then
    VOIDSCRIPT="./voidscript"
else
    echo "❌ VoidScript interpreter not found!"
    echo "Please compile the project first."
    exit 1
fi

echo "Using: $VOIDSCRIPT"
echo ""

# Run the quick test and capture output
echo "Running quick_empty_test.vs..."
OUTPUT=$($VOIDSCRIPT quick_empty_test.vs 2>&1)

echo "Raw output:"
echo "----------"
echo "$OUTPUT"
echo "----------"
echo ""

# Check for common issue patterns
ISSUES_FOUND=0

if echo "$OUTPUT" | grep -q "null"; then
    echo "❌ ISSUE: Found 'null' in output where empty string should be"
    ISSUES_FOUND=$((ISSUES_FOUND + 1))
fi

if echo "$OUTPUT" | grep -q '""'; then
    echo "❌ ISSUE: Found literal quotes in output"
    ISSUES_FOUND=$((ISSUES_FOUND + 1))
fi

# Check expected patterns
if echo "$OUTPUT" | grep -q "STARTEND"; then
    echo "✅ GOOD: print(\"\") appears to work correctly (no output between START and END)"
else
    echo "❌ ISSUE: print(\"\") may not be working correctly"
    ISSUES_FOUND=$((ISSUES_FOUND + 1))
fi

# Check for proper line breaks
LINE_COUNT=$(echo "$OUTPUT" | wc -l)
if [ $LINE_COUNT -ge 4 ]; then
    echo "✅ GOOD: printnl(\"\") appears to create line breaks"
else
    echo "❌ ISSUE: printnl(\"\") may not be creating proper line breaks"
    ISSUES_FOUND=$((ISSUES_FOUND + 1))
fi

echo ""
if [ $ISSUES_FOUND -eq 0 ]; then
    echo "🎉 ALL TESTS PASSED: Empty print functions appear to work correctly!"
    exit 0
else
    echo "⚠️  ISSUES FOUND: $ISSUES_FOUND problems detected with empty print functions"
    echo ""
    echo "This confirms the empty print issue exists. Check the output above for details."
    exit 1
fi