// Minimal OOP test for Curl functionality
printnl("=== Testing CurlClient OOP Interface (Minimal) ===");

// Test 1: Basic CurlClient creation
printnl("\n1. Creating CurlClient instance...");
$client = new CurlClient();
printnl("✓ CurlClient created successfully");

// Test 2: Simple GET request
printnl("\n2. Testing simple GET request...");
$response = $client->get("https://httpbin.org/get");
printnl("✓ GET request completed, response type: ", typeof($response));

printnl("\n=== CurlClient OOP Tests Completed ===");