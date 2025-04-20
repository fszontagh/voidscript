# Constants Feature Test
# Test declaration of immutable constants and verify re-assignment errors

# Declare constant string and print
const string $name = "Alice";
printnl($name);

# Declare constant integer and print
const int $x = 100;
printnl($x);

# Declare mutable variable and modify
string $y = "mutable";
printnl($y);
$y = "changed";
printnl($y);

# Attempt to modify constant (should produce runtime error)
// $name = "Bob";