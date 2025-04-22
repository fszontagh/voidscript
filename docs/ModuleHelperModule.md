 # ModuleHelperModule

 Provides helper functions to inspect and list dynamically loaded plugin modules.

 ## Functions

 ### module_list
 - **Signature:** `module_list() -> object`
 - **Description:** Returns an object array to module info objects for each loaded plugin module.
 - **Module info object fields:**
   - `name` (string): Module name (shared library filename without extension, `lib` prefix stripped).
   - `path` (string): Filesystem path to the loaded shared library.
   - `classes` (object): Map of index to class names registered by the module.
   - `functions` (object): Map of index to function names registered by the module.
   - `variables` (object): Map of index to variable names registered by the module (currently empty).
 - **Parameters:** None.
 - **Returns:** Object containing module info objects.
 - **Errors:** Throws if arguments are provided.

 ### module_exists
 - **Signature:** `module_exists(moduleName) -> bool`
 - **Description:** Checks whether a plugin module with the given name is loaded.
 - **Parameters:**
   - `moduleName` (string): Name of the module to check.
 - **Returns:** `true` if a loaded module matches `moduleName`, `false` otherwise.
 - **Errors:**
   - Throws if the argument count is not exactly one.
   - Throws if `moduleName` is not a string.

 ### module_info
 - **Signature:** `module_info(moduleName) -> object | null`
 - **Description:** Retrieves the info object for the specified module, as returned by `module_list()`.
 - **Parameters:**
   - `moduleName` (string): Name of the module.
 - **Returns:**
   - Module info object if found.
   - `null` if no module with that name is loaded.
 - **Errors:**
   - Throws if the argument count is not exactly one.
   - Throws if `moduleName` is not a string.

 ## Example

 ```vs
 // List all loaded modules


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


 // Check existence and get details

bool $CurlModuleExists = module_exists("CurlModule");

printnl("CurlModule exists: ", $CurlModuleExists);    # e.g., true or false

if ($CurlModuleExists) {
    object $info = module_info("CurlModule");
    printnl("Path: ", $info->path);
}

 ```