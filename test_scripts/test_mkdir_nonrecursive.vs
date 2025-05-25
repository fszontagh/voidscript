// Test mkdir non-recursive behavior
print("Testing mkdir non-recursive behavior\n");

// This should fail because parent directory doesn't exist
bool result = mkdir("nonexistent/child", false);

if (result) {
    print("✗ Non-recursive mkdir succeeded when it should have failed\n");
} else {
    print("✓ Non-recursive mkdir correctly failed for nested path\n");
}

// Create parent first
bool parent_result = mkdir("nonexistent");
if (parent_result) {
    print("✓ Created parent directory\n");
    
    // Now this should succeed
    bool child_result = mkdir("nonexistent/child", false);
    if (child_result) {
        print("✓ Non-recursive mkdir succeeded with existing parent\n");
    } else {
        print("✗ Non-recursive mkdir failed even with existing parent\n");
    }
    
    // Cleanup
    rmdir("nonexistent/child");
    rmdir("nonexistent");
    print("✓ Cleaned up test directories\n");
} else {
    print("✗ Could not create parent directory\n");
}

print("Test completed\n");
