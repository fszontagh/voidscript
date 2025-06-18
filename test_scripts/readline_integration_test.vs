# Test to verify ReadlineModule functions are properly registered
print("Testing ReadlineModule integration:");

# Check if functions exist by trying to access them through function_info
string[] functions = module_function_list("Readline");
int function_count = sizeof(functions);

if (function_count > 0) {
    print("ReadlineModule is properly integrated!");
    print("Available functions (" + number_to_string(function_count) + "):");
    for (int i = 0; i < function_count; i = i + 1) {
        print("  - " + functions[i]);
    }
} else {
    print("ERROR: ReadlineModule not found or no functions registered");
}

print("Integration test completed.");