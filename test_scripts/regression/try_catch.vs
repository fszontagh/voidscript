// Regression: bug #7 - try / catch / throw did not exist. There was no error recovery
// of any kind, so scripts had to validate everything up front and any runtime error
// killed the process. `try {` also misparsed into "Expected token PUNCTUATION with
// value ':'" because the parser guessed at a ternary.
// Expected: clean exit 0.

// --- catch with no variable (the form the existing test scripts use) ---
try {
    throw "boom";
    printnl("NOT REACHED");
} catch {
    printnl("caught-bare");
}

// --- catch binding the thrown value ---
try {
    throw "detailed message";
} catch (string $e) {
    printnl($e);                    // detailed message
}

// --- built-in runtime errors are catchable ---
try {
    int[] $a = [1, 2];
    printnl($a[0] + $a[1]);         // fine
    object $notAnArray = {};
    printnl($notAnArray->missing->deeper);
} catch (string $e) {
    printnl("caught-runtime");
}

// --- no throw means the catch block is skipped ---
try {
    printnl("try-ok");
} catch (string $e) {
    printnl("NOT REACHED");
}

// --- execution continues after the statement ---
printnl("after");

// --- nested try, inner catch handles it ---
try {
    try {
        throw "inner";
    } catch (string $e) {
        printnl("inner-caught");
    }
    printnl("outer-continues");
} catch (string $e) {
    printnl("NOT REACHED");
}

// --- an uncaught throw in an inner try propagates to the outer one ---
try {
    try {
        throw "propagate";
    } catch {
        throw "rethrown";
    }
} catch (string $e) {
    printnl($e);                    // rethrown
}

// --- try/catch inside a function, with return from both blocks ---
function attempt(boolean $fail) string {
    try {
        if ($fail) { throw "failed"; }
        return "ok";
    } catch (string $e) {
        return "recovered:" + $e;
    }
}
printnl(attempt(false));            // ok
printnl(attempt(true));             // recovered:failed

// --- catch must NOT swallow break: BreakException derives from std::exception ---
int $count = 0;
while (true) {
    try {
        $count = $count + 1;
        break;
    } catch (string $e) {
        printnl("NOT REACHED");
    }
}
printnl($count);                    // 1

// --- catch must NOT swallow return ---
function returnsThroughTry() string {
    try {
        return "returned";
    } catch (string $e) {
        return "NOT REACHED";
    }
}
printnl(returnsThroughTry());       // returned

printnl("done");
