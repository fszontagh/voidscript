// Basic GET request
const string $url = "https://jsonplaceholder.typicode.com/todos/1";
string $response = curlGet($url);
printnl("Basic GET response:", $response);

// GET with options - simplified to just use timeout
object $options = {
    int timeout: 10,
    boolean follow_redirects: true,
    object headers: {
        string "Accept": "application/json",
        string "UserAgent": "SoniScript/1.0"
    }
};

string $response2 = curlGet($url, $options);
printnl("GET with options response:", $response2);