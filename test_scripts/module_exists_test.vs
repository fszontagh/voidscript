print("Test 1: Starting");
print("Test 2: Module exists check");
bool $exists = module_exists("Math");
print("Test 3: Completed");
if ($exists) {
    print("Math module exists");
} else {
    print("Math module does not exist");
}