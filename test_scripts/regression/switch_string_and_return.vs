// Regression: two further switch defects.
//
// #16 - switch only accepted integer/enum expressions:
//   "Switch expression must evaluate to a non-null integer type"
// String dispatch is the single most common use for switch in a scripting
// language, so this made the statement useless for most real code.
//
// #17 - `return` inside a case body failed to parse with
//   "Cannot access token at end of stream"
// The case-body loop only stopped at 'case', 'default' and '}', so a return
// statement ran the parser off the end of the token stream.
//
// Expected: clean exit 0.

// --- string switch ---
string $cmd = "del";
switch ($cmd) {
    case "add": printnl("adding"); break;
    case "del": printnl("deleting"); break;
    default: printnl("unknown");
}

// no match falls to default
string $other = "zzz";
switch ($other) {
    case "add": printnl("NOT REACHED"); break;
    default: printnl("unknown-ok");
}

// string fallthrough
string $f = "a";
switch ($f) {
    case "a":
    case "b": printnl("a-or-b"); break;
    default: printnl("NOT REACHED");
}

// --- return inside a case ---
function pick(int $x) string {
    switch ($x) {
        case 1: return "one";
        case 2: return "two";
        default: return "other";
    }
}
printnl(pick(1));
printnl(pick(2));
printnl(pick(9));

// return inside a string switch, and code after the switch still reachable
function classify(string $s) string {
    switch ($s) {
        case "y": return "yes";
        default: break;
    }
    return "no";
}
printnl(classify("y"));
printnl(classify("q"));

// integer switch must still work
int $n = 2;
switch ($n) {
    case 1: printnl("NOT REACHED"); break;
    case 2: printnl("two-ok"); break;
    default: printnl("NOT REACHED");
}

printnl("done");

// --- #18: falsy values must be switchable ---
// ValuePtr::operator bool() returns VALUE truthiness, not pointer validity, so the
// guard `!switch_value` rejected any switch on 0 or "" as "non-null".
int $zero = 0;
switch ($zero) {
    case 0: printnl("zero-ok"); break;
    default: printnl("NOT REACHED");
}

string $empty = "";
switch ($empty) {
    case "": printnl("empty-ok"); break;
    default: printnl("NOT REACHED");
}

// enum member with an implicit value of 0 - what broke enum_switch.vs
enum Color { RED, GREEN = 5, BLUE };
switch (Color.RED) {
    case Color.RED: printnl("red-ok"); break;
    default: printnl("NOT REACHED");
}

printnl("falsy-done");
