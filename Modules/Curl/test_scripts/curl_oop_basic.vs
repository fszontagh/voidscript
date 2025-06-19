// Basic test for OOP Curl functionality
printnl("=== Testing CurlClient OOP Interface ===");

// Test 1: Basic CurlClient creation
printnl("\n1. Creating CurlClient instance...");
$client = new CurlClient();
printnl("✓ CurlClient created successfully");

// Test 2: Fluent API configuration
printnl("\n2. Testing fluent API configuration...");
$client->setTimeout(10)
       ->setDefaultHeader("User-Agent", "VoidScript-Curl/1.0")
       ->setFollowRedirects(true);
printnl("✓ Fluent API configuration completed");

// Test 3: Basic GET request to httpbin.org (if available)
printnl("\n3. Testing GET request...");
try {
    auto $response : $client->get("https://httpbin.org/get")
    
    printnl("Status Code: ", $response->getStatusCode());
    printnl("Success: ", $response->isSuccess());
    printnl("Total Time: ", $response->getTotalTime(), " seconds");
    printnl("Content-Type: ", $response->getHeader("Content-Type"));
    printnl("Body Length: ", $response->getBody()->length());
    
    if ($response->isSuccess()) {
        printnl("✓ GET request completed successfully");
    } else {
        printnl("✗ GET request failed: ", $response->getErrorMessage());
    }
} catch {
    printnl("✗ GET request failed with exception");
}

// Test 4: CurlClient with base URL
printnl("\n4. Testing CurlClient with base URL...");
try {
    auto $clientWithBase : new CurlClient();
    $clientWithBase->setBaseUrl("https://httpbin.org");
    auto $response2 : $clientWithBase->get("/user-agent");
    
    printnl("Base URL Status Code: ", $response2->getStatusCode());
    if ($response2->isSuccess()) {
        printnl("✓ Base URL request completed successfully");
    } else {
        printnl("✗ Base URL request failed: ", $response2->getErrorMessage());
    }
} catch {
    printnl("✗ Base URL request failed with exception");
}

// Test 5: POST request with data
printnl("\n5. Testing POST request...");
try {
    auto $postResponse : $client->post("https://httpbin.org/post", "{\"test\":\"data\"}");
    
    printnl("POST Status Code: ", $postResponse->getStatusCode());
    if ($postResponse->isSuccess()) {
        printnl("✓ POST request completed successfully");
    } else {
        printnl("✗ POST request failed: ", $postResponse->getErrorMessage());
    }
} catch {
    printnl("✗ POST request failed with exception");
}

// Test 6: Response toString method
printnl("\n6. Testing response toString method...");
try {
    auto $testResponse : $client->get("https://httpbin.org/status/200");
    auto $responseStr : $testResponse->toString();
    printnl("Response toString: ", $responseStr);
    printnl("✓ toString method working");
} catch {
    printnl("✗ toString test failed");
}

printnl("\n=== CurlClient OOP Tests Completed ===");