print("Testing if getline function exists...");
string $result = "";
try {
    $result = getline();
} catch {
    printnl("(no input available - skipping interactive test)");
}
print("getline returned: " + $result);