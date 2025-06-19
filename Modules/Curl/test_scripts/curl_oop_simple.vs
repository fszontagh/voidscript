// Simple test for OOP Curl functionality without network requests
printnl("=== Testing CurlClient OOP Interface (Basic) ===");

// Test 1: Basic CurlClient creation
printnl("\n1. Creating CurlClient instance...");
$client = new CurlClient();
printnl("✓ CurlClient created successfully");

// Test 2: Fluent API configuration
printnl("\n2. Testing fluent API configuration...");
$client->setTimeout(10);
$client->setDefaultHeader("User-Agent", "VoidScript-Curl/1.0");
$client->setFollowRedirects(true);
$client->setBaseUrl("https://example.com");
printnl("✓ Fluent API configuration completed");

// Test 3: CurlResponse creation
printnl("\n3. Creating CurlResponse instance...");
$response = new CurlResponse();
printnl("✓ CurlResponse created successfully");

// Test 4: Response methods (with default values)
printnl("\n4. Testing response methods with default values...");
printnl("Status Code: ", $response->getStatusCode());
printnl("Success: ", $response->isSuccess());
printnl("Total Time: ", $response->getTotalTime());
printnl("Error Message: ", $response->getErrorMessage());
printnl("Body Length: ", $response->getBody()->length());
printnl("✓ Response methods working");

printnl("\n=== Basic CurlClient OOP Tests Completed ===");