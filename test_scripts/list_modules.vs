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
            printnl("    - ", $func_info["documentation"]["name"]);
            if (sizeof($func_info["documentation"]["parameters"])>0) {
                printnl("      Parameters: ");
                for (string $param_name, object $param_info: $func_info["documentation"]["parameters"]) {
                    printnl("       - ",$param_info["name"], " type: ", $param_info["type"], " - ", $param_info["description"]);
                }
            }
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

