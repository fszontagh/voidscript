// Test script for ConversionModule error handling

printnl("=== Testing ConversionModule Error Handling ===");

// Test with empty string (should cause error)
printnl("Testing empty string...");
try {
    double $result = string_to_number("");
    printnl("FAIL: empty string should not have converted: ", $result);
} catch (string $e) {
    printnl("OK: rejected empty numeric input");
}