printnl("--- LOADED DYNAMIC MODULES ---");

object $z = module_list();
for (object $module : $z) {
    if (typeof($module,"object") == false) {
        throw_error("There is something wrong...");
    }

    printnl("Name: ", $module->name);
    printnl("Path: ", $module->path);

    if (sizeof($module->classes) > 0) {
        printnl("\t -- Registered classes: ", sizeof($module->classes));
        for (string $classes_name : $module->classes ) {
            printnl("\t\t - Name: ", $classes_name);
        }
    }

    if (sizeof($module->functions) > 0) {
        printnl("\t -- Registered functions: ", sizeof($module->functions));
        for (string $function_name : $module->functions ) {
            printnl("\t\t - Name: ", $function_name);
        }
    }


    if (sizeof($module->variables) > 0) {
        printnl("\t -- Registered variables: ", sizeof($module->variables));
        for (string $variable_name : $module->variables ) {
            printnl("\t\t - Name: ", $variable_name);
        }
    }

    // …
}
