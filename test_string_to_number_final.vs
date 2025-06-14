// Comprehensive test for the updated string_to_number function
printnl("=== VoidScript string_to_number Type Detection Test ===");
printnl("");

printnl("The updated string_to_number function now detects the appropriate");
printnl("numeric type based on the input string format:");
printnl("- Pure integers (e.g., '42') return int");
printnl("- Floats with 'f' suffix (e.g., '3.14f') return float");
printnl("- Decimals without suffix return float/double based on precision");
printnl("- Scientific notation returns double");
printnl("- Numbers too large for int return double");
printnl("");

// Test 1: Integer Detection
printnl("1. INTEGER DETECTION:");
int $int1 = string_to_number("42");
printnl("   string_to_number('42') -> int: " + number_to_string($int1));

int $int2 = string_to_number("-123");
printnl("   string_to_number('-123') -> int: " + number_to_string($int2));

int $int3 = string_to_number("2147483647");
printnl("   string_to_number('2147483647') -> int: " + number_to_string($int3));
printnl("");

// Test 2: Float Detection
printnl("2. FLOAT DETECTION:");
float $float1 = string_to_number("3.14f");
printnl("   string_to_number('3.14f') -> float: " + number_to_string($float1));

float $float2 = string_to_number("-2.5f");
printnl("   string_to_number('-2.5f') -> float: " + number_to_string($float2));

float $float3 = string_to_number("1.5");
printnl("   string_to_number('1.5') -> float: " + number_to_string($float3));
printnl("");

// Test 3: Double Detection
printnl("3. DOUBLE DETECTION:");
double $double1 = string_to_number("3.141592653589793");
printnl("   string_to_number('3.141592653589793') -> double: " + number_to_string($double1));

double $double2 = string_to_number("2147483648");
printnl("   string_to_number('2147483648') -> double: " + number_to_string($double2));

double $double3 = string_to_number("1.23e10");
printnl("   string_to_number('1.23e10') -> double: " + number_to_string($double3));
printnl("");

// Test 4: Edge Cases
printnl("4. EDGE CASES:");
int $zero = string_to_number("0");
printnl("   string_to_number('0') -> int: " + number_to_string($zero));

int $spaces = string_to_number("  42  ");
printnl("   string_to_number('  42  ') -> int: " + number_to_string($spaces));

double $scientific = string_to_number("-1.5E-5");
printnl("   string_to_number('-1.5E-5') -> double: " + number_to_string($scientific));
printnl("");

printnl("✓ All tests passed! The function correctly detects and returns");
printnl("  the appropriate numeric type based on input format.");
printnl("");
printnl("Note: When assigning to a specific type variable, the returned");
printnl("value must match that type, otherwise VoidScript will throw a");
printnl("type mismatch error (this is expected strict typing behavior).");