// Test control flow: if statements and loops
int $x = 10;

// If statement
if ($x > 5) {
    printnl("x is greater than 5");
} else {
    printnl("x is not greater than 5");
}

// While loop
int $i = 0;
printnl("While loop:");
while ($i < 3) {
    printnl("  i = ", $i);
    $i++;
}

// For loop
printnl("For loop:");
for (int $j = 0; $j < 3; $j++) {
    printnl("  j = ", $j);
}

// Nested control flow
printnl("Nested control flow:");
for (int $k = 0; $k < 3; $k++) {
    if ($k % 2 == 0) {
        printnl("  ", $k, " is even");
    } else {
        printnl("  ", $k, " is odd");
    }
}