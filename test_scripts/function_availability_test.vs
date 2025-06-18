# Test to check function availability
print("=== Function Availability Test ===");

# Test Math module functions
print("Math.PI exists: ");
double pi = PI();
print(number_to_string(pi));

print("Math.sqrt exists: ");
double sqrt_result = sqrt(4.0);
print(number_to_string(sqrt_result));

# Test if readline functions exist by checking module info
print("Checking available modules:");
string[] modules = module_list();
int module_count = sizeof(modules);
print("Available modules (" + number_to_string(module_count) + "):");
for (int i = 0; i < module_count; i = i + 1) {
    print("  - " + modules[i]);
}

print("Test completed.");