// Test script for ConversionModule invalid format handling

printnl("=== Testing ConversionModule Invalid Format ===");

// Test with invalid format (should cause error)
printnl("Testing invalid format...");
try {
    double $result = string_to_number("123abc");
    printnl("FAIL: invalid input should not have converted: ", $result);
} catch (string $e) {
    printnl("OK: rejected invalid numeric input");
}