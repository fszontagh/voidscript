// Feature: the interpreter passes unknown flags after the script (and everything after
// "--") into $argv, so scripts can implement their own flag-based CLI.
printnl($argc);
for (int $i = 1; $i < $argc; $i++) { printnl($argv[$i]); }
printnl("done");
