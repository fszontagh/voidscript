// Test error handling in Math module

printnl("=== Math Module Error Handling Test ===");
printnl("");

// Test sqrt with negative number
printnl("Testing sqrt(-1) - should cause error:");
try {
    double $result = sqrt(-1.0);
    printnl("ERROR: sqrt(-1) should have failed but returned: ", $result);
} catch {
    printnl("✓ sqrt(-1) correctly threw an error");
}

printnl("");
printnl("=== Error Handling Test Complete ===");