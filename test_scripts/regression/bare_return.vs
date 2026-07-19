// Regression: bug #2 - a function called in statement position whose body returns
// used to escape as an uncaught Interpreter::ReturnException and abort the process
// via std::terminate (SIGABRT), because CallStatementNode had no catch for it.
// Expected: clean exit 0, printing "a", "b", "42", "done".

// Untyped function with a bare early return.
function early() {
    printnl("a");
    return;
    printnl("NOT REACHED");
}

// Typed function (return type is postfix) called in statement position, so its
// value is discarded - this is the case that used to abort.
function withValue() int {
    printnl("b");
    return 42;
}

early();
withValue();

// The same function must still yield its value in expression position.
int $v = withValue();
printnl($v);

printnl("done");
