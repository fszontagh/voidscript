printnl("=== MODULE LISTING TEST ===");
printnl("\n--- DYNAMIC MODULES ---");

object $modules = module_list();
if (typeof($modules, "object") == false) {
    throw_error("module_list() did not return an object");
}

object $module_classes = {};
object $module_functions = {};
object $module_variables = {};

for (string $module_key, object $module : $modules) {
    if (typeof($module, "object") == false) {
        throw_error("Invalid module object in list");
    }

    printnl("\nModule: ", $module["name"]);
    printnl("Path: ", $module["path"]);

    $module_classes = $module["classes"];
    if (sizeof($module_classes) > 0) {
        printnl("\n  Classes (", sizeof($module_classes), "):");
        for (string $class_key, object $class_info : $module_classes) {
            printnl("    - ", $class_info["name"]);
        }
    }

    $module_functions = $module["functions"];
    if (sizeof($module_functions) > 0) {
        printnl("\n  Functions (", sizeof($module_functions), "):");
        for (string $func_key, object $func_info : $module_functions) {
            printnl("    - ", $func_info["name"]);
        }
    }

    $module_variables = $module["variables"];
    if (sizeof($module_variables) > 0) {
        printnl("\n  Variables (", sizeof($module_variables), "):");
        for (string $var_key, object $var_info : $module_variables) {
            printnl("    - ", $var_info["name"]);
        }
    }
    printnl("\n--------------------------------------------------");
}

printnl("\n--- MODULE EXISTENCE TESTS ---");

// Test dynamic module
string $test_module = "modules-curl";
bool $exists = module_exists($test_module);
printnl("\nChecking dynamic module '", $test_module, "':");
printnl("Exists: ", $exists);

if ($exists) {
    object $info = module_info($test_module);
    printnl("Module info:");
    printnl("  Name: ", $info["name"]);
    printnl("  Path: ", $info["path"]);
}

// Test built-in module
string $builtin_module = "ModuleHelper";
bool $builtin_exists = module_exists($builtin_module);
printnl("\nChecking built-in module '", $builtin_module, "':");
printnl("Exists: ", $builtin_exists);

if ($builtin_exists) {
    object $builtin_info = module_info($builtin_module);
    printnl("Module info:");
    printnl("  Name: ", $builtin_info["name"]);
    printnl("  Path: ", $builtin_info["path"]);
}

printnl("\n=== TEST COMPLETE ===");


