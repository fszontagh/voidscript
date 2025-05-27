printnl("=== TESTING CURL MODULE ===");

// Test if curlGet function exists and is callable  
object $options = {};
string $result = curlGet("https://httpbin.org/get", $options);
if (typeof($result, "string") == false) {
    throw_error("curlGet did not return a string");
}

printnl("curlGet test successful!");
printnl("Response length: ", string_length($result));

// Parse as JSON to verify structure
object $json_result = json_decode($result);
if (typeof($json_result, "object") == false) {
    throw_error("Response is not valid JSON");
}

printnl("JSON parsing successful!");
printnl("URL from response: ", $json_result["url"]);
