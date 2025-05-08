// This script is used to generate the docs for the loaded modules and the built-in modules

const object $module_list = module_list();


for (object $module : $module_list) {
    printnl("## " + $module->name);

    for (string $function : $module->functions) {
        printnl("- " + $function + "()");
    }
}