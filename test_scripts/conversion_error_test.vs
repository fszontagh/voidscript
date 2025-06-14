// Test script for ConversionModule error handling

printnl("=== Testing ConversionModule Error Handling ===");

// Test with empty string (should cause error)
printnl("Testing empty string...");
double $result = string_to_number("");
printnl("This line should not be reached!");