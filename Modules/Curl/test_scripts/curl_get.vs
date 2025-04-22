string $url = "https://jsonplaceholder.typicode.com/todos/1";

string $response = curlGet($url);

printnl("Response: ", $response);