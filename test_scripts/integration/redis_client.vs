// Integration test for the Redis module (needs a redis-server, e.g. docker run -p 6379:6379 redis).
// Not in the unattended ctest suite.
Redis $r = new Redis();
printnl("connected: ", $r->connect("127.0.0.1", 6379));
printnl("ping: ", $r->ping());
$r->set("vs:name", "alice");
printnl("get: ", $r->get("vs:name"));
printnl("exists: ", $r->exists("vs:name"));
printnl("missing is null: ", is_null($r->get("vs:nope")));
$r->del("vs:counter");
printnl("incr: ", $r->incr("vs:counter"));
$r->del("vs:list");
$r->command(["RPUSH", "vs:list", "a", "b", "c"]);
auto $items = $r->command(["LRANGE", "vs:list", "0", "-1"]);
printnl("list size: ", sizeof($items));
$r->del("vs:name"); $r->del("vs:counter"); $r->del("vs:list");
$r->close();
printnl("done");
