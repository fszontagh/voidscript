// Test script for Math module functionality

// Test PI constant
print("Testing PI():");
double $pi_value = PI();
print("PI = " + number_to_string($pi_value));

// Test basic functions
print("\nTesting basic math functions:");
double $test_num = 3.7;
print("Original number: " + number_to_string($test_num));
print("ceil(3.7) = " + number_to_string(ceil($test_num)));
print("floor(3.7) = " + number_to_string(floor($test_num)));
print("round(3.7) = " + number_to_string(round($test_num)));
print("abs(-3.7) = " + number_to_string(abs(-3.7)));

// Test square root
print("\nTesting sqrt:");
double $sqrt_test = 16.0;
print("sqrt(16) = " + number_to_string(sqrt($sqrt_test)));

// Test power function
print("\nTesting pow:");
double $base = 2.0;
double $exp = 3.0;
print("pow(2, 3) = " + number_to_string(pow($base, $exp)));

// Test trigonometric functions
print("\nTesting trigonometric functions:");
double $angle = $pi_value / 4.0; // 45 degrees in radians
print("sin(π/4) = " + number_to_string(sin($angle)));
print("cos(π/4) = " + number_to_string(cos($angle)));
print("tan(π/4) = " + number_to_string(tan($angle)));

// Test logarithmic functions
print("\nTesting logarithmic functions:");
double $log_test = 2.718282; // approximately e
print("log(e) ≈ " + number_to_string(log($log_test)));
print("log10(100) = " + number_to_string(log10(100.0)));

// Test min/max functions
print("\nTesting min/max functions:");
double $a = 5.5;
double $b = 3.2;
print("min(5.5, 3.2) = " + number_to_string(min($a, $b)));
print("max(5.5, 3.2) = " + number_to_string(max($a, $b)));

print("\nMath module test completed successfully!");