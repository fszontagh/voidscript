#!/usr/bin/env voidscript
printnl("=== VoidScript Documentation Generator ===");

string $docs_dir = "generated_docs";


if (!isset($argv[1])) {
    error("Usage: ", $argv[0], " target_dir");
    exit(1);
}

$docs_dir = $argv[1];


printnl("Generating documentation in directory: ", $docs_dir);

// Create main documentation directory
if (!file_exists($docs_dir)) {
    printnl("Creating documentation directory: ", $docs_dir);
    mkdir($docs_dir, true);
}

// Create subdirectories for builtin and dynamic modules
const string $builtin_dir = $docs_dir + "/builtin_modules";
const string $dynamic_dir = $docs_dir + "/dynamic_modules";

if (!file_exists($builtin_dir)) {
    mkdir($builtin_dir, true);
}

if (!file_exists($dynamic_dir)) {
    mkdir($dynamic_dir, true);
    printnl("Documentation directories created.");
}

function printMethods(object $class) {
    printnl("      * Methods: ");
    for (string $method_name, object $method: $class) {
        printnl("      ", $method_name);
    }
}


printnl("Analyzing modules...");

// Get all modules
object $modules = module_list();
if (typeof($modules, "object") == false) {
    throw_error("module_list() did not return an object");
}

// Generate documentation for each module
for (string $module_name, object $module : $modules) {
    printnl("Module: ", $module_name);
    object $info = module_info($module_name);
    if (!is_null($info["classes"])) {
        for (string $class_name, object $class : $info["classes"]) {
            printnl("class: ", $class_name);
            printnl(var_dump($class));
        }
    }
}

printnl("\n=== Documentation Generation Complete ===");
printnl("Documentation files generated in: ", $docs_dir);
printnl("- Built-in modules: ", $builtin_dir);
printnl("- Dynamic modules: ", $dynamic_dir);
printnl("\nTo view the documentation, check the generated .md files in each directory.");
