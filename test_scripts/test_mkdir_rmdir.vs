// Test script for mkdir and rmdir functions
print("Testing mkdir and rmdir functions\n");

// Test 1: Create a simple directory
print("Test 1: Creating a simple directory 'test_dir'\n");
bool result = mkdir("test_dir");
print("mkdir result: " + string(result) + "\n");

// Check if directory exists
if (file_exists("test_dir")) {
    print("✓ Directory 'test_dir' created successfully\n");
} else {
    print("✗ Failed to create directory 'test_dir'\n");
}

// Test 2: Try to remove the directory
print("\nTest 2: Removing directory 'test_dir'\n");
result = rmdir("test_dir");
print("rmdir result: " + string(result) + "\n");

// Check if directory still exists
if (!file_exists("test_dir")) {
    print("✓ Directory 'test_dir' removed successfully\n");
} else {
    print("✗ Failed to remove directory 'test_dir'\n");
}

// Test 3: Create nested directories recursively
print("\nTest 3: Creating nested directories 'parent/child/grandchild' recursively\n");
result = mkdir("parent/child/grandchild", true);
print("mkdir recursive result: " + string(result) + "\n");

// Check if nested directory exists
if (file_exists("parent/child/grandchild")) {
    print("✓ Nested directories created successfully\n");
} else {
    print("✗ Failed to create nested directories\n");
}

// Test 4: Try to create without recursive flag (should fail)
print("\nTest 4: Trying to create 'another/deep/path' without recursive flag\n");
result = mkdir("another/deep/path", false);
print("mkdir non-recursive result: " + string(result) + "\n");

if (!result) {
    print("✓ Correctly failed to create nested path without recursive flag\n");
} else {
    print("✗ Unexpectedly succeeded in creating nested path\n");
}

// Test 5: Clean up - remove nested directories (one by one, since rmdir only removes empty dirs)
print("\nTest 5: Cleaning up nested directories\n");
bool result1 = rmdir("parent/child/grandchild");
bool result2 = rmdir("parent/child");
bool result3 = rmdir("parent");

print("Cleanup results: " + string(result1) + ", " + string(result2) + ", " + string(result3) + "\n");

if (result1 && result2 && result3) {
    print("✓ All cleanup operations successful\n");
} else {
    print("✗ Some cleanup operations failed\n");
}

print("\nAll tests completed!\n");
