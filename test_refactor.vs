printnl("Testing ModuleHelperModule refactoring...");

printnl("\n1. Testing module_list():");
object $modules = module_list();
printnl("module_list() returned: " + typeof($modules));

printnl("\n2. Testing module_exists():");
bool $exists = module_exists("ModuleHelper");
if ($exists) {
    printnl("module_exists('ModuleHelper'): true");
} else {
    printnl("module_exists('ModuleHelper'): false");
}

printnl("\n3. Testing module_info():");
object $info = module_info("ModuleHelper");
printnl("module_info('ModuleHelper') returned: " + typeof($info));

printnl("\nAll tests completed successfully!");
