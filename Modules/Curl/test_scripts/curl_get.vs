// Basic GET request
const string $url = "https://jsonplaceholder.typicode.com/todos/1";
string $response = curlGet($url, {
    int timeout: 10,
    boolean follow_redirects: true,
    object headers: {
        string Accept: "application/json",
        string UserAgent: "VoidScript/1.0"
    }
});
printnl("Basic GET response:", $response);

//string $response2 = curlGet($url);
//printnl("GET with options response:", $response2);