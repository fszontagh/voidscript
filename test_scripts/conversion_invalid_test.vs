// Test script for ConversionModule invalid format handling

printnl("=== Testing ConversionModule Invalid Format ===");

// Test with invalid format (should cause error)
printnl("Testing invalid format...");
double $result = string_to_number("123abc");
printnl("This line should not be reached!");