// Basic DELETE request
string $url = "https://jsonplaceholder.typicode.com/posts/1";
string $response = curlDelete($url);
printnl("Basic DELETE response:", $response);

// DELETE with options
object $options = {
    "timeout": 10,
    "follow_redirects": true,
    "headers": {
        "Accept": "application/json",
        "User-Agent": "VoidScript/1.0"
    }
};

string $response2 = curlDelete($url, $options);
printnl("DELETE with options response:", $response2); 