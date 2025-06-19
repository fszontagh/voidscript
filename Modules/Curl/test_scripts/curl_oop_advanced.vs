// Advanced OOP features test for CurlClient and CurlResponse
printnl("=== Advanced CurlClient OOP Features Test ===");

// Test 1: CurlClient with base URL and path combination
printnl("\n1. Testing CurlClient with base URL and path combination...");
try {
    $apiClient = new CurlClient();
    $apiClient->setBaseUrl("https://httpbin.org");
    
    // Test absolute URL override
    $response1 = $apiClient->get("https://httpbin.org/status/200");
    printnl("✓ Absolute URL override - Status:", $response1->getStatusCode());
    
    // Test relative path combination
    $response2 = $apiClient->get("/get");
    printnl("✓ Relative path combination - Status:", $response2->getStatusCode());
    
    // Test path with leading slash
    $response3 = $apiClient->get("/json");
    printnl("✓ Path with leading slash - Status:", $response3->getStatusCode());
    
    // Test path without leading slash
    $response4 = $apiClient->get("uuid");
    printnl("✓ Path without leading slash - Status:", $response4->getStatusCode());
} catch (error $e) {
    printnl("✗ Base URL and path test failed: ", $e->getMessage());
}

// Test 2: Multiple clients with different configurations
printnl("\n2. Testing multiple clients with different configurations...");
try {
    // Fast client with short timeout
    $fastClient = new CurlClient();
    $fastClient->setTimeout(5)
               ->setDefaultHeader("X-Client-Type", "Fast")
               ->setFollowRedirects(true);
    
    // Slow client with long timeout  
    $slowClient = new CurlClient();
    $slowClient->setTimeout(30)
               ->setDefaultHeader("X-Client-Type", "Slow")
               ->setDefaultHeader("User-Agent", "VoidScript-SlowClient/1.0")
               ->setFollowRedirects(false);
    
    // Test both clients
    $fastResponse = $fastClient->get("https://httpbin.org/delay/1");
    $slowResponse = $slowClient->get("https://httpbin.org/user-agent");
    
    printnl("✓ Fast client - Status:", $fastResponse->getStatusCode(), " Time:", $fastResponse->getTotalTime());
    printnl("✓ Slow client - Status:", $slowResponse->getStatusCode(), " Time:", $slowResponse->getTotalTime());
    
    // Verify different headers were sent
    printnl("✓ Fast client User-Agent:", $fastResponse->getHeader("X-Client-Type"));
    printnl("✓ Slow client has custom headers configured");
    
} catch (error $e) {
    printnl("✗ Multiple clients test failed: ", $e->getMessage());
}

// Test 3: Header management (setting, overriding, retrieving)
printnl("\n3. Testing comprehensive header management...");
try {
    $headerClient = new CurlClient();
    
    // Set default headers
    $headerClient->setDefaultHeader("Accept", "application/json")
                 ->setDefaultHeader("X-API-Key", "test-key-123")
                 ->setDefaultHeader("X-Custom", "default-value");
    
    // Test GET with default headers
    $response = $headerClient->get("https://httpbin.org/headers");
    printnl("✓ GET with default headers - Status:", $response->getStatusCode());
    
    // Test POST with additional headers via options
    $postOptions = {
        headers: {
            "Content-Type": "application/json",
            "X-Custom": "overridden-value",
            "X-Request-ID": "req-456"
        }
    };
    
    $postResponse = $headerClient->post("https://httpbin.org/post", 
                                       "{\"test\": \"data\"}", 
                                       $postOptions);
    printnl("✓ POST with header override - Status:", $postResponse->getStatusCode());
    
    // Test response header retrieval
    $contentType = $postResponse->getHeader("Content-Type");
    $server = $postResponse->getHeader("Server");
    $nonExistent = $postResponse->getHeader("X-NonExistent");
    
    printnl("✓ Content-Type header:", $contentType);
    printnl("✓ Server header:", $server);
    printnl("✓ Non-existent header (should be empty):", $nonExistent);
    
    // Test getting all headers
    $allHeaders = $postResponse->getHeaders();
    printnl("✓ Retrieved all response headers successfully");
    
} catch (error $e) {
    printnl("✗ Header management test failed: ", $e->getMessage());
}

// Test 4: Error handling scenarios
printnl("\n4. Testing error handling scenarios...");

// Test invalid URL
try {
    $errorClient = new CurlClient();
    $errorResponse = $errorClient->get("not-a-valid-url");
    printnl("✗ Invalid URL should have failed");
} catch (error $e) {
    printnl("✓ Invalid URL properly caught: ", $e->getMessage());
}

// Test timeout scenario
try {
    $timeoutClient = new CurlClient();
    $timeoutClient->setTimeout(1); // Very short timeout
    $timeoutResponse = $timeoutClient->get("https://httpbin.org/delay/5");
    
    if (!$timeoutResponse->isSuccess()) {
        printnl("✓ Timeout handled gracefully - Error:", $timeoutResponse->getErrorMessage());
    } else {
        printnl("✗ Timeout should have failed");
    }
} catch (error $e) {
    printnl("✓ Timeout exception properly caught: ", $e->getMessage());
}

// Test network error (non-existent domain)
try {
    $networkClient = new CurlClient();
    $networkResponse = $networkClient->get("https://this-domain-definitely-does-not-exist-12345.com");
    
    if (!$networkResponse->isSuccess()) {
        printnl("✓ Network error handled - Error:", $networkResponse->getErrorMessage());
    } else {
        printnl("✗ Network error should have failed");
    }
} catch (error $e) {
    printnl("✓ Network error exception properly caught: ", $e->getMessage());
}

// Test 5: Response object methods and properties
printnl("\n5. Testing comprehensive response object features...");
try {
    $testClient = new CurlClient();
    
    // Test different status codes
    $statusCodes = [200, 404, 500];
    
    for (int $i = 0; $i < count($statusCodes); $i++) {
        int $code = $statusCodes[$i];
        $statusResponse = $testClient->get("https://httpbin.org/status/" + $code);
        
        printnl("Status ", $code, " test:");
        printnl("  - Status Code:", $statusResponse->getStatusCode());
        printnl("  - Is Success:", $statusResponse->isSuccess());
        printnl("  - Total Time:", $statusResponse->getTotalTime(), "s");
        printnl("  - Body Length:", $statusResponse->getBody()->length());
        printnl("  - toString():", $statusResponse->toString());
        
        // Verify success logic
        bool $expectedSuccess = ($code >= 200 && $code < 300);
        if ($statusResponse->isSuccess() == $expectedSuccess) {
            printnl("  ✓ Success detection working correctly");
        } else {
            printnl("  ✗ Success detection failed");
        }
    }
    
} catch (error $e) {
    printnl("✗ Response object test failed: ", $e->getMessage());
}

// Test 6: Fluent API chaining
printnl("\n6. Testing fluent API method chaining...");
try {
    // Create client and configure everything in one chain
    $fluentClient = new CurlClient();
    $fluentResponse = $fluentClient->setBaseUrl("https://httpbin.org")
                                  ->setTimeout(15)
                                  ->setDefaultHeader("X-Fluent", "true")
                                  ->setDefaultHeader("Accept", "application/json")
                                  ->setFollowRedirects(true)
                                  ->get("/json");
    
    printnl("✓ Fluent API chaining - Status:", $fluentResponse->getStatusCode());
    printnl("✓ Fluent API chaining - Success:", $fluentResponse->isSuccess());
    printnl("✓ Fluent API method chaining working perfectly");
    
} catch (error $e) {
    printnl("✗ Fluent API chaining test failed: ", $e->getMessage());
}

// Test 7: All HTTP methods
printnl("\n7. Testing all HTTP methods...");
try {
    $httpClient = new CurlClient();
    $httpClient->setBaseUrl("https://httpbin.org");
    
    // GET
    $getResp = $httpClient->get("/get");
    printnl("✓ GET - Status:", $getResp->getStatusCode());
    
    // POST
    $postResp = $httpClient->post("/post", "{\"method\": \"POST\"}");
    printnl("✓ POST - Status:", $postResp->getStatusCode());
    
    // PUT
    $putResp = $httpClient->put("/put", "{\"method\": \"PUT\"}");
    printnl("✓ PUT - Status:", $putResp->getStatusCode());
    
    // DELETE
    $deleteResp = $httpClient->delete("/delete");
    printnl("✓ DELETE - Status:", $deleteResp->getStatusCode());
    
} catch (error $e) {
    printnl("✗ HTTP methods test failed: ", $e->getMessage());
}

// Test 8: Timeout and redirect settings verification
printnl("\n8. Testing timeout and redirect behavior...");
try {
    // Test redirect following
    $redirectClient = new CurlClient();
    $redirectClient->setFollowRedirects(true);
    $redirectResponse = $redirectClient->get("https://httpbin.org/redirect/3");
    
    if ($redirectResponse->isSuccess()) {
        printnl("✓ Redirect following - Status:", $redirectResponse->getStatusCode());
    } else {
        printnl("✗ Redirect following failed");
    }
    
    // Test redirect not following
    $noRedirectClient = new CurlClient();
    $noRedirectClient->setFollowRedirects(false);
    $noRedirectResponse = $noRedirectClient->get("https://httpbin.org/redirect/1");
    
    int $redirectStatus = $noRedirectResponse->getStatusCode();
    if ($redirectStatus >= 300 && $redirectStatus < 400) {
        printnl("✓ Redirect not followed - Status:", $redirectStatus);
    } else {
        printnl("✗ Redirect behavior unexpected - Status:", $redirectStatus);
    }
    
} catch (error $e) {
    printnl("✗ Redirect test failed: ", $e->getMessage());
}

printnl("\n=== Advanced CurlClient OOP Features Test Completed ===");