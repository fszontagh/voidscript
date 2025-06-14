// Test script for ConversionModule functions

printnl("=== Testing ConversionModule ===");

// Test string_to_number function
printnl("Testing string_to_number:");

// Test valid integer strings
double $result1 = string_to_number("123");
printnl("string_to_number('123') = " + number_to_string($result1));

// Test valid floating-point strings
double $result2 = string_to_number("123.456");
printnl("string_to_number('123.456') = " + number_to_string($result2));

// Test negative numbers
double $result3 = string_to_number("-42");
printnl("string_to_number('-42') = " + number_to_string($result3));

// Test negative floating-point
double $result4 = string_to_number("-3.14159");
printnl("string_to_number('-3.14159') = " + number_to_string($result4));

// Test with whitespace
double $result5 = string_to_number("  42.5  ");
printnl("string_to_number('  42.5  ') = " + number_to_string($result5));

printnl("");
printnl("Testing number_to_string:");

// Test integer conversion
int $int_value = 42;
string $str1 = number_to_string($int_value);
printnl("number_to_string(42) = '" + $str1 + "'");

// Test double conversion
double $double_value = 3.14159;
string $str2 = number_to_string($double_value);
printnl("number_to_string(3.14159) = '" + $str2 + "'");

// Test negative number conversion
double $neg_value = -123.45;
string $str3 = number_to_string($neg_value);
printnl("number_to_string(-123.45) = '" + $str3 + "'");

printnl("");
printnl("Testing round-trip conversion:");

// Test round-trip: string -> number -> string
string $original = "987.654";
double $as_number = string_to_number($original);
string $back_to_string = number_to_string($as_number);
printnl("Original: '" + $original + "', Round-trip: '" + $back_to_string + "'");

printnl("");
printnl("=== ConversionModule tests completed ===");