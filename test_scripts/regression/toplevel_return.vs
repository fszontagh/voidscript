// Regression: bug #11 - VoidScript::run() caught only std::exception, and
// ReturnException does not derive from it, so a top-level `return` escaped main
// and aborted the process via std::terminate (exit 134, "terminate called").
//
// A top-level return now ends the script cleanly, as it does in PHP.
// Expected: clean exit 0, printing "before" and nothing after.

printnl("before");
return;
printnl("NOT REACHED");
