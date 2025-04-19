# Expressions Feature Test
int $a = 10;
int $b = 3;

printnl("Startig variables");
printnl("$a = ", $a, "\n$b = ", $b);
printnl("--------------------------------");
print("a + b = ", $a + $b, "\n");
print("a - b = ", $a - $b, "\n");
printnl("$b - $a = ", $b - $a);
print("a * b = ", $a * $b, "\n");
print("a / b = ", $a / $b, "\n");
print("a % b = ", $a % $b, "\n");

boolean $eq = ($a == $b);
boolean $neq = ($a != $b);
boolean $gt = ($a > $b);
boolean $lt = ($a < $b);
boolean $gte = ($a >= $b);
boolean $lte = ($a <= $b);

print("eq = ", $eq, "\n");
print("neq = ", $neq, "\n");
print("gt = ", $gt, "\n");
print("lt = ", $lt, "\n");
print("gte = ", $gte, "\n");
print("lte = ", $lte, "\n");

boolean $logical = ($a > 5) && ($b < 5);
print("logical = ", $logical, "\n");

boolean $not = !($a == $b);
print("not = ", $not, "\n");