// Test mkdir and rmdir functions
print("Testing mkdir and rmdir functions\n");

// Test 1: Create a simple directory
print("Test 1: Creating directory 'test_dir'\n");
bool result1 = mkdir("test_dir");
if (result1) {
    print("✓ mkdir returned true\n");
} else {
    print("✗ mkdir returned false\n");
}

// Check if directory exists
if (file_exists("test_dir")) {
    print("✓ Directory exists\n");
} else {
    print("✗ Directory does not exist\n");
}

// Test 2: Remove directory
print("\nTest 2: Removing directory 'test_dir'\n");
bool result2 = rmdir("test_dir");
if (result2) {
    print("✓ rmdir returned true\n");
} else {
    print("✗ rmdir returned false\n");
}

// Check if directory still exists
if (!file_exists("test_dir")) {
    print("✓ Directory removed successfully\n");
} else {
    print("✗ Directory still exists\n");
}

// Test 3: Recursive mkdir
print("\nTest 3: Creating nested directories recursively\n");
bool result3 = mkdir("parent/child/grandchild", true);
if (result3) {
    print("✓ Recursive mkdir succeeded\n");
} else {
    print("✗ Recursive mkdir failed\n");
}

if (file_exists("parent/child/grandchild")) {
    print("✓ Nested directories exist\n");
} else {
    print("✗ Nested directories do not exist\n");
}

// Test 4: Cleanup
print("\nTest 4: Cleaning up nested directories\n");
bool cleanup1 = rmdir("parent/child/grandchild");
bool cleanup2 = rmdir("parent/child");
bool cleanup3 = rmdir("parent");

if (cleanup1 && cleanup2 && cleanup3) {
    print("✓ All directories cleaned up successfully\n");
} else {
    print("✗ Some directories could not be removed\n");
}

print("\nAll tests completed!\n");
