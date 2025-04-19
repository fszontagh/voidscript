# CurlModule

This module provides HTTP GET and POST functionality via libcurl in VoidScript.

## Functions

### curlGet
`curlGet(url [, options]) -> string`

- `url` (string): The HTTP/HTTPS URL to request.
- `options` (object, optional): Configuration object with fields:
  - `timeout` (int or float): Maximum time in seconds to wait for the request.
  - `follow_redirects` (bool): Whether to follow HTTP redirects (default: false).
  - `headers` (object): Custom HTTP headers as key/value pairs. Keys must be valid identifiers (no hyphens).

Returns the response body as a string.

### curlPost
`curlPost(url, data [, options]) -> string`

- `url` (string): The HTTP/HTTPS URL to send the POST request.
- `data` (string): Request body (e.g., a JSON-encoded payload).
- `options` (object, optional): Same as for `curlGet`. If `headers` does not include a `Content-Type`,
  `Content-Type: application/json` is automatically added.

Returns the response body as a string.

## Examples

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

Basic POST (JSON payload):
```vs
object $payload = { string title: "foo", string body: "bar", int userId: 1 };
string $json = json_encode($payload);
string $resp = curlPost("https://jsonplaceholder.typicode.com/posts", $json);
printnl($resp);
```

POST with options:
```vs
object $payload = { string name: "Alice", int age: 30 };
string $json = json_encode($payload);
string $resp = curlPost("https://example.com/api/users", $json, {
    timeout: 10,
    follow_redirects: true,
    headers: {
        X_Test: "CustomValue"
    }
});
printnl($resp);
```

## Integration

Ensure the `CurlModule` is registered before running scripts:
```cpp
Modules::ModuleManager::instance().addModule(
    std::make_unique<Modules::CurlModule>()
);
Modules::ModuleManager::instance().registerAll();
```

Place this file alongside `CMakeLists.txt` and `src/` in the `Modules/CurlModule/` folder.