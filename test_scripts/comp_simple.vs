// Function that returns a comparison result
function isGreater(int $a, int $b) bool {
    return $a > $b;
}

function isAdult(int $age) bool {
    return $age >= 18;
}

// Test basic comparisons
printnl("10 > 5: ", isGreater(10, 5));
printnl("5 > 10: ", isGreater(5, 10));

printnl("20 is adult: ", isAdult(20));
printnl("15 is adult: ", isAdult(15)); 