// Regression: bug #14 - the ternary conditional `?:` did not exist. `$x > 3 ? "a" : "b"`
// was a syntax error, and because the parser guessed at a ternary when it saw an
// unexpected token, its absence also produced the misleading
// "Expected token PUNCTUATION with value ':'" for unrelated mistakes such as `try {`.
// Expected: clean exit 0.

int $n = 5;

// basic
printnl($n > 3 ? "big" : "small");          // big
printnl($n < 3 ? "big" : "small");          // small

// in a declaration
string $label = $n > 3 ? "over" : "under";
printnl($label);                            // over

// numeric result
int $clamped = $n > 10 ? 10 : $n;
printnl($clamped);                          // 5

// nested in the else branch (right associative, as in C)
int $g = 75;
printnl($g >= 90 ? "A" : $g >= 70 ? "B" : "C");   // B

// lower precedence than the operators around it: this is ($n + 1) > 5, not $n + (1 > 5)
printnl($n + 1 > 5 ? "yes" : "no");         // yes

// works as a call argument
printnl(format("{}", $n > 0 ? "pos" : "neg"));   // pos

// only the taken branch is evaluated
function boom() int { throw_error("branch should not be evaluated"); return 0; }
printnl($n > 3 ? 1 : boom());               // 1

printnl("done");
