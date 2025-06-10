// Test file with syntax errors - should fail compilation
int $a = 10;
string $b = "Hello"  // Missing semicolon
bool $c = true;

function badFunction(int $x {  // Missing closing parenthesis
    return $x + 1;
}

// Unclosed block
if ($a > 5) {
    printnl("Test");
// Missing closing brace

printnl("This should not compile");