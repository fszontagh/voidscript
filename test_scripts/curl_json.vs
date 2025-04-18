# CURL + JSON Feature Test
# GET request
string $url = "https://jsonplaceholder.typicode.com/todos/1";
string $resp = curlGet($url);
printnl("GET raw: ", $resp);
object $data = json_decode($resp);
printnl("ID: ", $data->id, " Title: ", $data->title, " Completed: ", $data->completed);

# POST request
string $postUrl = "https://jsonplaceholder.typicode.com/posts";
object $payload = {
    string title: "foo",
    string body: "bar",
    int userId: 1
};
string $postData = json_encode($payload);
printnl("POST data: ", $postData);
string $postResp = curlPost($postUrl, $postData);
printnl("POST raw: ", $postResp);
object $postJson = json_decode($postResp);
// The response from JSONPlaceholder includes only the new id
printnl("POST ID: ", $postJson->id);
// --- GET with options ---
printnl("GET with options (timeout=5, follow_redirects, Accept header):");
string $optGet = curlGet($url, {
    timeout: 5,
    follow_redirects: true,
    headers: { Accept: "application/json" }
});
printnl($optGet);
// --- POST with options ---
printnl("POST with options (timeout=5, follow_redirects, X_Test header):");
string $optPost = curlPost($postUrl, $postData, {
    timeout: 5,
    follow_redirects: true,
    headers: { X_Test: "CustomValue" }
});
printnl($optPost);