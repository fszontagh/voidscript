string $url = "https://jsonplaceholder.typicode.com/posts";

string $postData = "{\"title\":\"foo\",\"body\":\"bar\",\"userId\":1}";

printnl("Posting data: '",$postData,"' to URL: ", $url);
string $response = curlPost($url, $postData);

printnl("Response: ", $response);