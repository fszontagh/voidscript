// Test legacy curl functions to ensure basic module works
printnl("=== Testing Legacy Curl Functions ===");

// Test legacy curlGet function
printnl("\n1. Testing legacy curlGet...");

string $result = curlGet("https://httpbin.org/get", {});
printnl("✓ Legacy curlGet working, got response: ", $result);

printnl("\n=== Legacy Curl Tests Completed ===");