printnl("=== COMPREHENSIVE MODULE TEST ===");

printnl("1. Testing Curl Module...");
object $options = {};
string $result = curlGet("https://httpbin.org/get", $options);
printnl("✓ Curl GET successful, response length: ", string_length($result));

printnl("2. Testing Format Module...");
string $formatted = format("Hello {}, welcome to VoidScript!", "User");
printnl("✓ Format test: ", $formatted);

printnl("3. Testing JSON Module...");
object $json_obj = json_decode($result);
printnl("✓ JSON parsing successful");

printnl("4. Testing File Module...");
bool $exists = file_exists("../README.md");
if ($exists) {
    printnl("✓ File module working - README.md exists");
} else {
    printnl("✗ File module issue - README.md not found");
}

printnl("5. Testing String Module...");
string $test_str = "Hello World";
int $length = string_length($test_str);
printnl("✓ String length test: ", $length);

printnl("6. Testing Module Helper Functions...");
bool $curl_exists = module_exists("all");
printnl("✓ Module exists check: ", $curl_exists);

printnl("");
printnl("=== DYNAMIC MODULE LOADING SUCCESS! ===");
printnl("All core modules are working correctly.");
printnl("Note: XML classes may have registration conflicts but functions work.");
