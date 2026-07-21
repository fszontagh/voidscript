MemcachedConnection $m1 = new MemcachedConnection("127.0.0.1:21211");
MemcachedConnection $m2 = new MemcachedConnection("127.0.0.1:21211");

// functional
$m1->set("greeting", "hello", 60);
printnl("m1 get greeting: ", $m1->get("greeting"));

// m2 is an independent client to the same server
printnl("m2 sees greeting (same server): ", $m2->get("greeting"));

// independence: disconnect m1, m2 must still work (if they collided, m2 dies too)
$m1->disconnect();
printnl("m1 isConnected after disconnect: ", $m1->isConnected());   // false
printnl("m2 isConnected: ", $m2->isConnected());                     // true - independent
$m2->set("k2", "world", 60);
printnl("m2 get k2: ", $m2->get("k2"));
printnl("done");
