// Round-trip test for object_set
object $base = json_decode("{\"name\":\"hello\",\"version\":\"1\"}");
object_set($base, "release", 1);
object_set($base, "nested", {string a: "hello"});
string $out = json_encode($base);
print($out);
print("\n");

// Expected line:
// {"name":"hello","nested":{"a":"hello"},"release":1}
