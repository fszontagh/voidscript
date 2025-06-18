print("Testing module_exists function:");

// Test known modules
bool $mathExists = module_exists("Math");
if ($mathExists) {
    print("Math module exists: true");
} else {
    print("Math module exists: false");
}

bool $stringExists = module_exists("String");
if ($stringExists) {
    print("String module exists: true");
} else {
    print("String module exists: false");
}

bool $fileExists = module_exists("File");
if ($fileExists) {
    print("File module exists: true");
} else {
    print("File module exists: false");
}

// Test unknown module
bool $unknownExists = module_exists("UnknownModule");
if ($unknownExists) {
    print("UnknownModule exists: true");
} else {
    print("UnknownModule exists: false");
}

// Test other ModuleHelper functions still work
print("Testing list_modules:");
auto $modules = list_modules();
print("Number of modules: " + sizeof($modules));

print("All tests completed successfully!");