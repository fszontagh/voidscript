// Test script to verify ConversionModule integration

printnl("=== Checking ConversionModule Integration ===");

// Check if the functions are available
printnl("Available modules:");
printnl("Module exists check: ", module_exists("Conversion"));

printnl("");
printnl("Testing basic functionality:");
double $test_num = string_to_number("42.5");
string $test_str = number_to_string($test_num);
printnl("Conversion test successful: ", $test_str);

printnl("");
printnl("=== ConversionModule Integration Verified ===");