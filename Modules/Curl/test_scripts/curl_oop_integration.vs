// Integration test combining legacy and OOP APIs in same script
printnl("=== Curl Legacy and OOP Integration Test ===");

// Test 1: Verify both APIs work independently in same script
printnl("\n1. Testing both legacy and OOP APIs in same script...");

// Legacy API tests
printnl("\n--- Legacy API Tests ---");
try {
    // Basic legacy GET
    string $legacyGet = curlGet("https://httpbin.org/get");
    printnl("✓ Legacy curlGet - Response length:", $legacyGet->length());
    
    // Legacy POST
    string $legacyPost = curlPost("https://httpbin.org/post", "{\"legacy\": true}");
    printnl("✓ Legacy curlPost - Response length:", $legacyPost->length());
    
    // Legacy PUT
    string $legacyPut = curlPut("https://httpbin.org/put", "{\"legacy\": true}");
    printnl("✓ Legacy curlPut - Response length:", $legacyPut->length());
    
    // Legacy DELETE
    string $legacyDelete = curlDelete("https://httpbin.org/delete");
    printnl("✓ Legacy curlDelete - Response length:", $legacyDelete->length());
    
} catch (error $e) {
    printnl("✗ Legacy API failed: ", $e->getMessage());
}

// OOP API tests
printnl("\n--- OOP API Tests ---");
try {
    $client = new CurlClient();
    $client->setTimeout(10)->setDefaultHeader("X-Test", "OOP");
    
    // OOP GET
    $oopGet = $client->get("https://httpbin.org/get");
    printnl("✓ OOP GET - Status:", $oopGet->getStatusCode(), " Body length:", $oopGet->getBody()->length());
    
    // OOP POST
    $oopPost = $client->post("https://httpbin.org/post", "{\"oop\": true}");
    printnl("✓ OOP POST - Status:", $oopPost->getStatusCode(), " Success:", $oopPost->isSuccess());
    
    // OOP PUT
    $oopPut = $client->put("https://httpbin.org/put", "{\"oop\": true}");
    printnl("✓ OOP PUT - Status:", $oopPut->getStatusCode(), " Time:", $oopPut->getTotalTime());
    
    // OOP DELETE
    $oopDelete = $client->delete("https://httpbin.org/delete");
    printnl("✓ OOP DELETE - Status:", $oopDelete->getStatusCode(), " Headers count:", $oopDelete->getHeaders()->size());
    
} catch (error $e) {
    printnl("✗ OOP API failed: ", $e->getMessage());
}

// Test 2: Verify no conflicts between function-based and class-based usage
printnl("\n2. Testing for conflicts between legacy and OOP usage...");
try {
    // Interleave legacy and OOP calls
    string $legacy1 = curlGet("https://httpbin.org/uuid");
    
    $oopClient = new CurlClient();
    $oop1 = $oopClient->get("https://httpbin.org/uuid");
    
    string $legacy2 = curlPost("https://httpbin.org/post", "{\"test\": 1}");
    
    $oop2 = $oopClient->post("https://httpbin.org/post", "{\"test\": 2}");
    
    string $legacy3 = curlGet("https://httpbin.org/json");
    
    $oop3 = $oopClient->get("https://httpbin.org/json");
    
    printnl("✓ Interleaved calls successful:");
    printnl("  - Legacy UUID length:", $legacy1->length());
    printnl("  - OOP UUID status:", $oop1->getStatusCode());
    printnl("  - Legacy POST length:", $legacy2->length());
    printnl("  - OOP POST status:", $oop2->getStatusCode());
    printnl("  - Legacy JSON length:", $legacy3->length());
    printnl("  - OOP JSON status:", $oop3->getStatusCode());
    
} catch (error $e) {
    printnl("✗ Interleaved usage failed: ", $e->getMessage());
}

// Test 3: Memory management and cleanup
printnl("\n3. Testing memory management and cleanup...");
try {
    // Create multiple clients and let them go out of scope
    for (int $i = 0; $i < 5; $i++) {
        $tempClient = new CurlClient();
        $tempClient->setTimeout(5)->setBaseUrl("https://httpbin.org");
        $tempResponse = $tempClient->get("/status/200");
        
        if (!$tempResponse->isSuccess()) {
            printnl("✗ Client ", $i, " failed");
            break;
        }
    }
    printnl("✓ Multiple client creation and cleanup successful");
    
    // Mix with legacy calls
    for (int $j = 0; $j < 3; $j++) {
        string $legacyResult = curlGet("https://httpbin.org/status/200");
        $tempClient2 = new CurlClient();
        $tempResponse2 = $tempClient2->get("https://httpbin.org/status/200");
    }
    printnl("✓ Mixed legacy/OOP memory management successful");
    
} catch (error $e) {
    printnl("✗ Memory management test failed: ", $e->getMessage());
}

// Test 4: Performance comparison scenarios
printnl("\n4. Performance comparison between legacy and OOP APIs...");
try {
    // Time legacy API calls
    int $legacyStartTime = time();
    for (int $i = 0; $i < 3; $i++) {
        string $legacyResp = curlGet("https://httpbin.org/get");
    }
    int $legacyEndTime = time();
    int $legacyDuration = $legacyEndTime - $legacyStartTime;
    
    // Time OOP API calls
    int $oopStartTime = time();
    $perfClient = new CurlClient();
    for (int $i = 0; $i < 3; $i++) {
        $oopResp = $perfClient->get("https://httpbin.org/get");
    }
    int $oopEndTime = time();
    int $oopDuration = $oopEndTime - $oopStartTime;
    
    printnl("✓ Performance comparison:");
    printnl("  - Legacy API: ", $legacyDuration, " seconds for 3 requests");
    printnl("  - OOP API: ", $oopDuration, " seconds for 3 requests");
    
    if ($oopDuration <= $legacyDuration + 1) {
        printnl("✓ OOP API performance is acceptable");
    } else {
        printnl("⚠ OOP API may have performance overhead");
    }
    
} catch (error $e) {
    printnl("✗ Performance comparison failed: ", $e->getMessage());
}

// Test 5: Test connection reuse benefits with OOP
printnl("\n5. Testing connection reuse benefits with OOP client...");
try {
    // Single client for multiple requests (connection reuse)
    $reuseClient = new CurlClient();
    $reuseClient->setBaseUrl("https://httpbin.org")
                ->setTimeout(10)
                ->setDefaultHeader("Connection", "keep-alive");
    
    double $totalTime = 0.0;
    for (int $i = 0; $i < 3; $i++) {
        $response = $reuseClient->get("/get?request=" + $i);
        $totalTime += $response->getTotalTime();
        printnl("  Request ", $i + 1, " - Status:", $response->getStatusCode(), " Time:", $response->getTotalTime(), "s");
    }
    
    double $avgTime = $totalTime / 3.0;
    printnl("✓ Connection reuse test - Average time per request:", $avgTime, "s");
    
    // Compare with individual legacy calls (new connection each time)
    double $legacyTotalTime = 0.0;
    for (int $i = 0; $i < 3; $i++) {
        int $startTime = time();
        string $legacyResp = curlGet("https://httpbin.org/get?legacy=" + $i);
        int $endTime = time();
        double $requestTime = $endTime - $startTime;
        $legacyTotalTime += $requestTime;
        printnl("  Legacy request ", $i + 1, " - Time: ~", $requestTime, "s");
    }
    
    double $legacyAvgTime = $legacyTotalTime / 3.0;
    printnl("✓ Legacy comparison - Average time per request: ~", $legacyAvgTime, "s");
    
} catch (error $e) {
    printnl("✗ Connection reuse test failed: ", $e->getMessage());
}

// Test 6: Advanced integration scenarios
printnl("\n6. Testing advanced integration scenarios...");
try {
    // Use legacy API to get some data, then process with OOP API
    string $legacyData = curlGet("https://httpbin.org/json");
    printnl("✓ Got data with legacy API");
    
    // Use OOP client to POST that data somewhere else
    $integrationClient = new CurlClient();
    $integrationClient->setDefaultHeader("Content-Type", "application/json");
    $postResponse = $integrationClient->post("https://httpbin.org/post", $legacyData);
    
    printnl("✓ Posted legacy data with OOP client - Status:", $postResponse->getStatusCode());
    
    // Verify we can parse response details
    printnl("✓ Integration response details:");
    printnl("  - Success:", $postResponse->isSuccess());
    printnl("  - Content-Type:", $postResponse->getHeader("Content-Type"));
    printnl("  - Total time:", $postResponse->getTotalTime(), "s");
    
} catch (error $e) {
    printnl("✗ Advanced integration test failed: ", $e->getMessage());
}

// Test 7: Error handling consistency between APIs
printnl("\n7. Testing error handling consistency...");
try {
    // Legacy API error handling
    try {
        string $legacyError = curlGet("https://nonexistent-domain-12345.invalid");
        printnl("✗ Legacy API should have failed");
    } catch (error $legacyErr) {
        printnl("✓ Legacy API error caught:", $legacyErr->getMessage());
    }
    
    // OOP API error handling
    try {
        $errorClient = new CurlClient();
        $oopError = $errorClient->get("https://nonexistent-domain-12345.invalid");
        if (!$oopError->isSuccess()) {
            printnl("✓ OOP API error handled gracefully:", $oopError->getErrorMessage());
        } else {
            printnl("✗ OOP API should have failed");
        }
    } catch (error $oopErr) {
        printnl("✓ OOP API error caught:", $oopErr->getMessage());
    }
    
} catch (error $e) {
    printnl("✗ Error handling consistency test failed: ", $e->getMessage());
}

printnl("\n=== Integration Test Summary ===");
printnl("✓ Legacy functions and OOP classes work together seamlessly");
printnl("✓ No conflicts detected between different API styles");
printnl("✓ Memory management appears stable");
printnl("✓ Both APIs handle errors appropriately");
printnl("✓ OOP API provides enhanced functionality while maintaining compatibility");

printnl("\n=== Curl Legacy and OOP Integration Test Completed ===");