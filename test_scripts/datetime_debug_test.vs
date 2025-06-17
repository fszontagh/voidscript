print("=== DateTime Debug Test ===");

// Test built-in functions first
print("Testing built-in functions:");
int $timestamp = current_unix_timestamp();
print("Timestamp: " + number_to_string($timestamp));
string $date_str = date();
print("Date: " + $date_str);

// Test DateTime object creation
print("");
print("Testing DateTime object creation:");
DateTime $dt = new DateTime();
print("DateTime object created successfully");

// Let's check what type it is
print("Type check: " + typeof($dt));

// Try to see if we can call any method
print("Attempting to call methods...");

// Simple test for debugging
print("End of debug test");