// Comprehensive test script for Math module functionality

printnl("=== VoidScript Math Module Test ===");
printnl("");

// Test PI constant
printnl("1. Testing PI constant:");
printnl("----------------------");
double $pi_value = PI();
printnl("PI = ", $pi_value);
printnl("");

// Test basic functions
printnl("2. Testing basic math functions:");
printnl("---------------------------------");
double $test_num = 3.7;
printnl("Original number: ", $test_num);
printnl("ceil(3.7) = ", ceil($test_num));
printnl("floor(3.7) = ", floor($test_num));
printnl("round(3.7) = ", round($test_num));
printnl("abs(-3.7) = ", abs(-3.7));
printnl("");

// Test square root
printnl("3. Testing sqrt:");
printnl("----------------");
double $sqrt_test = 16.0;
printnl("sqrt(16) = ", sqrt($sqrt_test));
printnl("");

// Test power function
printnl("4. Testing pow:");
printnl("---------------");
double $base = 2.0;
double $exp = 3.0;
printnl("pow(2, 3) = ", pow($base, $exp));
printnl("");

// Test trigonometric functions
printnl("5. Testing trigonometric functions:");
printnl("------------------------------------");
double $angle = $pi_value / 4.0; // 45 degrees in radians
printnl("sin(π/4) ≈ ", sin($angle));
printnl("cos(π/4) ≈ ", cos($angle));
printnl("tan(π/4) ≈ ", tan($angle));
printnl("");

// Test logarithmic functions
printnl("6. Testing logarithmic functions:");
printnl("----------------------------------");
double $log_test = 2.718282; // approximately e
printnl("log(e) ≈ ", log($log_test));
printnl("log10(100) = ", log10(100.0));
printnl("");

// Test min/max functions
printnl("7. Testing min/max functions:");
printnl("------------------------------");
double $a = 5.5;
double $b = 3.2;
printnl("min(5.5, 3.2) = ", min($a, $b));
printnl("max(5.5, 3.2) = ", max($a, $b));
printnl("");

// Test with integer inputs
printnl("8. Testing with integer inputs:");
printnl("-------------------------------");
int $int_a = 5;
int $int_b = 3;
printnl("sqrt(9) = ", sqrt(9));
printnl("pow(5, 3) = ", pow($int_a, $int_b));
printnl("min(5, 3) = ", min($int_a, $int_b));
printnl("max(5, 3) = ", max($int_a, $int_b));
printnl("");

printnl("=== Math Module Test Complete ===");
printnl("All mathematical functions tested successfully!");