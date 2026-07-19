// Regression: bug #3 - switch parsed fine and had a working SwitchStatementNode,
// but the parser emitted Operations::Type::ControlFlow which Interpreter::runOperation
// had no case for, so every switch died with "Unknown operation type".
// Expected: clean exit 0, printing "two", "fallthrough-hit", "default-hit", "done".

int $x = 2;
switch ($x) {
    case 1:
        printnl("one");
        break;
    case 2:
        printnl("two");
        break;
    default:
        printnl("other");
}

// A case with no break must fall through to the next case body.
int $y = 1;
switch ($y) {
    case 1:
    case 2:
        printnl("fallthrough-hit");
        break;
    default:
        printnl("NOT REACHED");
}

// No matching case must run the default block.
int $z = 99;
switch ($z) {
    case 1:
        printnl("NOT REACHED");
        break;
    default:
        printnl("default-hit");
}

printnl("done");
