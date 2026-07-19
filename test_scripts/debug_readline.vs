# Debug version to understand the buffering issue

printnl("DEBUG: About to call readline()");
string $result = "";
try {
    $result = readline("Type ONE line: ");
} catch {
    printnl("(no input available - skipping interactive test)");
}
printnl("DEBUG: readline() returned: [" + $result + "]");
printnl("DEBUG: Test complete");