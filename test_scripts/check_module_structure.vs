printnl("=== CHECKING MODULE_LIST STRUCTURE ===");

object $modules = module_list();
printnl("Type of modules: ", typeof($modules, "object"));

// Check if it's an array or object
int $size = sizeof($modules);
printnl("Size of modules: ", $size);

// Let's see what keys exist by trying some common ones
printnl("Trying to access index 0:");
if (typeof($modules[0], "object")) {
    printnl("Found object at index 0");
} else {
    printnl("No object at index 0, value: ", $modules[0]);
}

// Let's try a different approach - since the output shows module names,
// maybe we need to get more info about the structure
printnl("Let's check what module_info returns:");
object $info = module_info("all");
printnl("Module info result: ", typeof($info, "object"));
if (typeof($info, "object")) {
    printnl("Module info for 'all':");
    printnl($info);
}
