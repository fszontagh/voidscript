# Debug version to understand the buffering issue

printnl("DEBUG: About to call readline()");
string $result = readline("Type ONE line: ");
printnl("DEBUG: readline() returned: [" + $result + "]");
printnl("DEBUG: Test complete");