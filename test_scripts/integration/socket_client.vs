// Integration test for the TcpClient built-in (needs a listening server). Not in the
// unattended ctest suite. Example: start `python3 -m http.server 8000` then run this.
string $host = "127.0.0.1";
int    $port = 8000;

TcpClient $c = new TcpClient();
printnl("connected: ", $c->connect($host, $port, 5));
$c->send("GET / HTTP/1.0\r\nHost: localhost\r\n\r\n");

string $status = string_trim($c->recvLine());
printnl("status: ", $status);                      // e.g. HTTP/1.0 200 OK

// drain the rest
string $body = "";
string $chunk = $c->recv(4096);
while (string_length($chunk) > 0) {
    $body = $body + $chunk;
    $chunk = $c->recv(4096);
}
printnl("bytes: ", string_length($body));
$c->close();
printnl("connected after close: ", $c->isConnected());
printnl("done");
