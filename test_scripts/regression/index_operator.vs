// Regression: bug #4 - '[' was registered as an infix binary operator in the
// shunting-yard expression parser rather than a postfix accessor, so it bound as
// a Binary node. That produced four unrelated-looking failures:
//   $q->outputs[0]  -> "Invalid member access expression - right side has kind: Binary"
//   $a[0] = 9       -> "Unsupported types in binary expression: object and int (a [] 0)"
//   $a[] = 3        -> "Expression could not be parsed cleanly"
// Expected: clean exit 0.

// --- index read (this always worked; guard against regressing it) ---
int[] $a = [10, 20, 30];
printnl($a[1]);                 // 20

// --- indexed assignment ---
$a[0] = 99;
printnl($a[0]);                 // 99

// --- member access chained with indexing ---
object $q = {};
$q->outputs = [10, 20, 30];
printnl($q->outputs[2]);        // 30

// --- indexed assignment through a member ---
$q->outputs[1] = 77;
printnl($q->outputs[1]);        // 77

// --- nested index ---
object $deep = {};
$deep->rows = [[1, 2], [3, 4]];
printnl($deep->rows[1][0]);     // 3

// --- index by a computed expression ---
int $i = 1;
printnl($a[$i + 1]);            // 30

printnl("done");

// --- append: $a[] = x (PHP-style push onto the end) ---
int[] $list = [1, 2];
$list[] = 3;
$list[] = 4;
printnl($list[2]);              // 3
printnl($list[3]);              // 4

// append through a member
object $bag = {};
$bag->items = [7];
$bag->items[] = 8;
printnl($bag->items[1]);        // 8

printnl("append-ok");
