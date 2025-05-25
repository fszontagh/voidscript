printnl("=== CURL FUNCTION DOCUMENTATION ===");

// Get function documentation for curlGet
object $doc = function_doc("curlGet");
printnl("Function: ", $doc["name"]);
printnl("Description: ", $doc["description"]);
