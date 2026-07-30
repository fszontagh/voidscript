# Redis module

A minimal [Redis](https://redis.io) client as the VoidScript class `Redis`, speaking the
RESP protocol over a raw TCP socket - **no external dependency** (no hiredis). On by
default.

```voidscript
Redis $r = new Redis();
$r->connect("127.0.0.1", 6379);       // host/port/timeout all optional
$r->set("name", "alice");
printnl($r->get("name"));             // alice
printnl($r->exists("name"));          // true
$r->incr("counter");                  // -> 1
$r->expire("name", 60);
printnl($r->ping());                  // PONG
// escape hatch for any command; returns the raw reply (string/int/array/null)
auto $reply = $r->command(["LPUSH", "mylist", "x", "y"]);
$r->close();
```

Methods: `connect([host[,port[,timeout]]])`, `isConnected()`, `set(k,v)`, `get(k)` (null if
missing), `del(k)`, `exists(k)`, `incr(k)`, `expire(k,seconds)`, `ping()`, `command(array)`,
`close()`. A `-ERR` reply throws (catch with `try`/`catch`). Values are sent as their string
form. Each instance owns its own connection, keyed by the framework instance id.
