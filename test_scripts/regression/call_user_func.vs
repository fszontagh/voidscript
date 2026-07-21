// Regression: call_user_func(name, ...args) invokes a user-defined function by name.
// This is the native->script bridge (Interpreter::callUserFunction) that lets native
// modules call back into script code - e.g. a live progress handler. Exposed as a
// built-in so the mechanism is testable without a module.
// Expected: clean exit 0.

function greet(string $who) string {
    return "hi " + $who;
}
function noret(int $n) {
    printnl("n=", $n);
}

// return value propagates
printnl(call_user_func("greet", "world"));       // hi world

// void function, side effect only
call_user_func("noret", 7);                       // n=7

// dynamic dispatch: the name comes from a variable
string $fn = "greet";
printnl(call_user_func($fn, "dynamic"));          // hi dynamic

// calling inside a loop (many invocations)
int $i = 0;
while ($i < 3) {
    printnl(call_user_func("greet", number_to_string($i)));
    $i = $i + 1;
}

// a bad function name is a catchable error, not a crash
try {
    call_user_func("does_not_exist");
    printnl("NOT REACHED");
} catch (string $e) {
    printnl("caught missing fn");
}

printnl("done");
