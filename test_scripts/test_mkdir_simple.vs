// Simple test for mkdir and rmdir functions
print("Testing mkdir and rmdir functions\n");

// Test 1: Create a simple directory
print("Test 1: Creating directory 'test_dir'\n");
bool result = mkdir("test_dir");
if (result) {
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
result = rmdir("test_dir");
if (result) {
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
result = mkdir("parent/child/grandchild", true);
if (result) {
    print("✓ Recursive mkdir succeeded\n");
} else {
    print("✗ Recursive mkdir failed\n");
}

if (file_exists("parent/child/grandchild")) {
    print("✓ Nested directories exist\n");
} else {
    print("✗ Nested directories do not exist\n");
}

print("\nAll tests completed!\n");
