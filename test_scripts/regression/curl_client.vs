// Regression: task #8 - the OOP CurlClient was completely unusable and three of the
// four free functions were never registered. Four separate defects:
//
//  1. parseOptions did `if (!options || ...)`, and ValuePtr's bool conversion THROWS on
//     a genuinely null value - so any request without an options argument died with
//     "Cannot convert NULL value (bool operator)" before doing anything.
//  2. mergeOptions returned ValuePtr(map, true), which tags the map as CLASS, but
//     parseOptions requires OBJECT - so every request rejected the options it had just
//     built with "options must be an object".
//  3. createResponse wrote the class marker as "__class__", but the interpreter reads
//     "$class_name" - so every method on a CurlResponse failed with
//     "Object missing $class_name property".
//  4. curlPost, curlPut and curlDelete were implemented but never registered, leaving
//     GET as the only reachable verb.
//
// Uses a file:// URL so the test is hermetic - no network, no listening port.
// Expected: clean exit 0.

string $path = "/tmp/voidscript_curl_regression.json";
file_put_contents($path, "{\"ok\":true}", true);
string $url = "file://" + $path;

// free function
printnl(curlGet($url));                 // {"ok":true}

// all four free functions must be registered
printnl(function_exists("curlGet"));    // true
printnl(function_exists("curlPost"));   // true
printnl(function_exists("curlPut"));    // true
printnl(function_exists("curlDelete")); // true

// OOP client, with no options argument - this is what used to throw
CurlClient $c = new CurlClient();
CurlResponse $r = $c->get($url);
printnl($r->getBody());                 // {"ok":true}

// Two responses must be independent - they used to collide because every CurlResponse
// serialises to the same $class_name and the data map was keyed by that.
file_put_contents($path, "{\"which\":\"first\"}", true);
CurlResponse $r1 = $c->get($url);
file_put_contents($path, "{\"which\":\"second\"}", true);
CurlResponse $r2 = $c->get($url);
printnl($r1->getBody());                 // {"which":"first"} - not overwritten by r2
printnl($r2->getBody());                 // {"which":"second"}

file_unlink($path);
printnl("done");
