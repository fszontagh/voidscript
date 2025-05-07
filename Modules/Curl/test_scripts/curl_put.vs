const object $options = {
    int timeout: 10,
    bool follow_redirects: true,
    object headers: {
        string ContentType: "application/json",
        string Accept: "application/json",
        string UserAgent: "VoidScript/1.0"
    }
};

string $url = "https://jsonplaceholder.typicode.com/posts/1";
string $data = '{"id": 1, "title": "Updated Post", "body": "This post has been updated", "userId": 1}';
string $response = curlPut($url, $data, $options);
printnl("Basic PUT response:", $response);


string $response2 = curlPut($url, $data, $options);
printnl("PUT with options response:", $response2); 
