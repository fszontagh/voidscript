// Test script for Math module functionality

// Test PI constant
print("Testing Math.PI():");
double pi_value = Math.PI();
print("PI = " + string_to_number(pi_value));

// Test basic functions
print("\nTesting basic math functions:");
double test_num = 3.7;
print("Original number: " + string_to_number(test_num));
print("ceil(3.7) = " + string_to_number(Math.ceil(test_num)));
print("floor(3.7) = " + string_to_number(Math.floor(test_num)));
print("round(3.7) = " + string_to_number(Math.round(test_num)));
print("abs(-3.7) = " + string_to_number(Math.abs(-3.7)));

// Test square root
print("\nTesting sqrt:");
double sqrt_test = 16.0;
print("sqrt(16) = " + string_to_number(Math.sqrt(sqrt_test)));

// Test power function
print("\nTesting pow:");
double base = 2.0;
double exp = 3.0;
print("pow(2, 3) = " + string_to_number(Math.pow(base, exp)));

// Test trigonometric functions
print("\nTesting trigonometric functions:");
double angle = pi_value / 4.0; // 45 degrees in radians
print("sin(π/4) = " + string_to_number(Math.sin(angle)));
print("cos(π/4) = " + string_to_number(Math.cos(angle)));
print("tan(π/4) = " + string_to_number(Math.tan(angle)));

// Test logarithmic functions
print("\nTesting logarithmic functions:");
double log_test = 2.718282; // approximately e
print("log(e) ≈ " + string_to_number(Math.log(log_test)));
print("log10(100) = " + string_to_number(Math.log10(100.0)));

// Test min/max functions
print("\nTesting min/max functions:");
double a = 5.5;
double b = 3.2;
print("min(5.5, 3.2) = " + string_to_number(Math.min(a, b)));
print("max(5.5, 3.2) = " + string_to_number(Math.max(a, b)));

print("\nMath module test completed successfully!");