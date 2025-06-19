# CurlModule

This module provides comprehensive HTTP client functionality via libcurl in VoidScript, offering both legacy function-based API and modern object-oriented interface.

## Overview

The CurlModule supports two distinct APIs:

1. **Legacy Functions**: Simple function-based interface (`curlGet`, `curlPost`, etc.) for basic HTTP operations
2. **Object-Oriented Interface**: Modern class-based API with `CurlClient` and `CurlResponse` classes for advanced features

The OOP interface provides enhanced functionality including:
- Connection reuse and performance optimization
- Rich response objects with detailed information
- Fluent API for method chaining
- Advanced header management
- Comprehensive error handling
- Request/response timing information

## Legacy Functions API

### curlGet
`curlGet(url [, options]) -> string`

- `url` (string): The HTTP/HTTPS URL to request.
- `options` (object, optional): Configuration object with fields:
  - `timeout` (int or float): Maximum time in seconds to wait for the request.
  - `follow_redirects` (bool): Whether to follow HTTP redirects (default: false).
  - `headers` (object): Custom HTTP headers as key/value pairs.

Returns the response body as a string.

### curlPost
`curlPost(url, data [, options]) -> string`

- `url` (string): The HTTP/HTTPS URL to send the POST request.
- `data` (string): Request body (e.g., a JSON-encoded payload).
- `options` (object, optional): Same as for `curlGet`.

Returns the response body as a string.

### curlPut
`curlPut(url, data [, options]) -> string`

- `url` (string): The HTTP/HTTPS URL to send the PUT request.
- `data` (string): Request body.
- `options` (object, optional): Same as for `curlGet`.

Returns the response body as a string.

### curlDelete
`curlDelete(url [, options]) -> string`

- `url` (string): The HTTP/HTTPS URL to send the DELETE request.
- `options` (object, optional): Same as for `curlGet`.

Returns the response body as a string.

## Object-Oriented API

### CurlClient Class

The `CurlClient` class provides an advanced HTTP client with connection reuse, fluent API, and detailed response information.

#### Constructors

```vs
// Create a basic client
$client = new CurlClient();

// Create a client with base URL
$client = new CurlClient();
$client->setBaseUrl("https://api.example.com");
```

#### Configuration Methods (Fluent API)

All configuration methods return the client instance for method chaining:

```vs
$client->setBaseUrl("https://api.example.com")
        ->setTimeout(30)
        ->setDefaultHeader("User-Agent", "MyApp/1.0")
        ->setDefaultHeader("Accept", "application/json")
        ->setFollowRedirects(true);
```

**Available Configuration Methods:**
- `setBaseUrl(url)`: Set base URL for relative requests
- `setTimeout(seconds)`: Set request timeout in seconds
- `setDefaultHeader(name, value)`: Set a default header for all requests
- `setFollowRedirects(follow)`: Enable/disable automatic redirect following

#### HTTP Methods

All HTTP methods return a `CurlResponse` object:

```vs
// GET request
$response = $client->get("/users");
$response = $client->get("/users", options);

// POST request  
$response = $client->post("/users", jsonData);
$response = $client->post("/users", jsonData, options);

// PUT request
$response = $client->put("/users/123", jsonData);
$response = $client->put("/users/123", jsonData, options);

// DELETE request
$response = $client->delete("/users/123");
$response = $client->delete("/users/123", options);
```

### CurlResponse Class

The `CurlResponse` class provides detailed information about HTTP responses.

#### Properties and Methods

```vs
// Status information
int $status = $response->getStatusCode();        // HTTP status code
bool $success = $response->isSuccess();          // true for 2xx status codes
string $error = $response->getErrorMessage();    // Error message if failed
double $time = $response->getTotalTime();        // Total request time in seconds

// Response content
string $body = $response->getBody();             // Response body content
object $headers = $response->getHeaders();       // All response headers
string $header = $response->getHeader("Content-Type"); // Specific header

// Utility methods
object $json = $response->asJson();              // Parse body as JSON
string $str = $response->toString();             // String representation
```

## Usage Examples

### Legacy API Examples

Basic GET:
```vs
string $resp = curlGet("https://jsonplaceholder.typicode.com/todos/1");
printnl($resp);
```

GET with options:
```vs
string $resp = curlGet("https://jsonplaceholder.typicode.com/todos/1", {
    timeout: 5,
    follow_redirects: true,
    headers: {
        Accept: "application/json"
    }
});
printnl($resp);
```

Basic POST:
```vs
object $payload = { string title: "foo", string body: "bar", int userId: 1 };
string $json = json_encode($payload);
string $resp = curlPost("https://jsonplaceholder.typicode.com/posts", $json);
printnl($resp);
```

### OOP API Examples

Basic client usage:
```vs
$client = new CurlClient();
$response = $client->get("https://api.github.com/users/octocat");

if ($response->isSuccess()) {
    printnl("Status: ", $response->getStatusCode());
    printnl("Content-Type: ", $response->getHeader("Content-Type"));
    printnl("Response time: ", $response->getTotalTime(), "s");
    printnl("Body length: ", $response->getBody()->length(), " bytes");
} else {
    printnl("Request failed: ", $response->getErrorMessage());
}
```

Fluent API with base URL:
```vs
$apiClient = new CurlClient();
$apiClient->setBaseUrl("https://jsonplaceholder.typicode.com")
          ->setTimeout(10)
          ->setDefaultHeader("User-Agent", "VoidScript/1.0")
          ->setFollowRedirects(true);

// All requests will use the base URL
$user = $apiClient->get("/users/1");
$posts = $apiClient->get("/posts?userId=1");
```

Advanced POST with headers:
```vs
$client = new CurlClient();
$client->setDefaultHeader("Content-Type", "application/json")
       ->setDefaultHeader("X-API-Key", "your-api-key");

$userData = "{\"name\": \"John Doe\", \"email\": \"john@example.com\"}";
$response = $client->post("https://api.example.com/users", $userData);

if ($response->isSuccess()) {
    printnl("User created successfully!");
    printnl("Location: ", $response->getHeader("Location"));
} else {
    printnl("Failed to create user: ", $response->getStatusCode());
}
```

Error handling:
```vs
$client = new CurlClient();
$client->setTimeout(5);

try {
    $response = $client->get("https://api.example.com/data");
    
    if ($response->isSuccess()) {
        // Process successful response
        string $data = $response->getBody();
    } else {
        // Handle HTTP errors (4xx, 5xx)
        printnl("HTTP Error: ", $response->getStatusCode());
        printnl("Error: ", $response->getErrorMessage());
    }
} catch (error $e) {
    // Handle network errors, timeouts, etc.
    printnl("Network error: ", $e->getMessage());
}
```

Connection reuse for multiple requests:
```vs
$client = new CurlClient();
$client->setBaseUrl("https://api.github.com")
       ->setTimeout(30)
       ->setDefaultHeader("Accept", "application/json");

// Reuse connection for multiple requests
$users = $client->get("/users");
$repos = $client->get("/repositories");
$events = $client->get("/events");

printnl("Users request: ", $users->getTotalTime(), "s");
printnl("Repos request: ", $repos->getTotalTime(), "s");  
printnl("Events request: ", $events->getTotalTime(), "s");
```

## Migration Guide

### From Legacy to OOP API

**Legacy:**
```vs
string $response = curlGet("https://api.example.com/data", {
    timeout: 10,
    headers: { Accept: "application/json" }
});
```

**OOP Equivalent:**
```vs
$client = new CurlClient();
$response = $client->setTimeout(10)
                   ->setDefaultHeader("Accept", "application/json")
                   ->get("https://api.example.com/data");
string $body = $response->getBody();
```

**Benefits of Migration:**
- Access to response status codes and headers
- Better error handling and diagnostics
- Connection reuse for improved performance
- Fluent API for cleaner code
- Detailed timing information

### Gradual Migration

You can use both APIs in the same script during migration:

```vs
// Legacy API for simple requests
string $quickData = curlGet("https://api.example.com/quick");

// OOP API for complex scenarios
$client = new CurlClient();
$client->setBaseUrl("https://api.example.com");
$detailedResponse = $client->get("/detailed");

if ($detailedResponse->isSuccess()) {
    // Process detailed response
}
```

## Best Practices

### Performance Optimization

1. **Reuse CurlClient instances** for multiple requests to the same domain
2. **Set appropriate timeouts** based on expected response times
3. **Use base URLs** to avoid repeating common URL prefixes
4. **Enable redirect following** when appropriate

### Error Handling

1. **Always check `isSuccess()`** before processing response body
2. **Use try-catch blocks** for network-level errors
3. **Check status codes** for specific HTTP error handling
4. **Log `getTotalTime()`** for performance monitoring

### Security

1. **Validate URLs** before making requests
2. **Use HTTPS** for sensitive data
3. **Set reasonable timeouts** to prevent hanging
4. **Sanitize headers** and request data

### Memory Management

1. **Let CurlClient instances go out of scope** when done
2. **Don't store unnecessary response data** in long-running scripts
3. **Use appropriate timeout values** to prevent resource leaks

## Backward Compatibility

The new OOP interface is fully compatible with existing legacy function usage. All existing scripts using `curlGet()`, `curlPost()`, etc. will continue to work without modification.

The legacy functions are implemented using the same underlying `CurlClient` class, ensuring consistent behavior and performance.

## Integration

Ensure the `CurlModule` is registered before running scripts:

```cpp
Modules::ModuleManager::instance().addModule(
    std::make_unique<Modules::CurlModule>()
);
Modules::ModuleManager::instance().registerAll();
```

The module automatically registers both legacy functions and OOP classes when loaded.

## Testing

Comprehensive test scripts are available in the `test_scripts/` directory:

- `curl_legacy_test.vs` - Basic legacy function tests
- `curl_oop_basic.vs` - Basic OOP functionality tests  
- `curl_oop_advanced.vs` - Advanced OOP features tests
- `curl_oop_integration.vs` - Integration between legacy and OOP APIs
- `curl_oop_errors.vs` - Error handling scenarios
- `curl_performance_test.vs` - Performance benchmarks

Run these tests to verify functionality after building the module.