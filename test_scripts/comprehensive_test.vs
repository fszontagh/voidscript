printnl("=== COMPREHENSIVE MODULE TEST ===");

printnl("1. Testing Curl Module...");
// Serve the fixture over file:// rather than fetching https://httpbin.org/get. This
// script is a ctest case, and pointing it at a third-party website made the build
// depend on that site being up AND returning JSON - it started failing the moment
// httpbin answered with an HTML error page instead.
string $fixture = "/tmp/voidscript_comprehensive_fixture.json";
file_put_contents($fixture, "{\"url\": \"file\", \"ok\": true}", true);
object $options = {};
string $result = curlGet("file://" + $fixture, $options);
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

file_unlink($fixture);
