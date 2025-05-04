// Basic POST request
string $url = "https://jsonplaceholder.typicode.com/posts";
string $data = '{"title": "Test Post", "body": "This is a test post", "userId": 1}';
string $response = curlPost($url, $data);
printnl("Basic POST response:", $response);

// POST with options
const object $options = {
    int timeout: 10,
    bool follow_redirects: true,
    object $headers: {
        string ContentType: "application/json",
        string Accept: "application/json",
        string UserAgent: "SoniScript/1.0"
    }
};

string $response2 = curlPost($url, $data, $options);
printnl("POST with options response:", $response2);