// Roadmap Tier 1: the Array module had only sizeof(). Adds map/filter/reduce/sort/usort/
// keys/values/reverse/slice/merge/unique/flip/in_array. Callbacks are by function name.
// Note sort is numeric-aware (not the lexical "10" < "2" of the underlying ObjectMap).
function dbl(int $x) int { return $x * 2; }
function isEven(int $x) boolean { return ($x % 2) == 0; }
function add(int $c, int $x) int { return $c + $x; }
function desc(int $a, int $b) int { return $b - $a; }

int[] $a = [3, 1, 2, 10, 5];
printnl(json_encode(array_map($a, "dbl")));        // [6,2,4,20,10]
printnl(json_encode(array_filter($a, "isEven")));  // [2,10]
printnl(array_reduce($a, "add", 0));               // 21
printnl(json_encode(array_sort($a)));              // [1,2,3,5,10]
printnl(json_encode(array_usort($a, "desc")));     // [10,5,3,2,1]
printnl(json_encode(array_reverse($a)));           // [5,10,2,1,3]
printnl(json_encode(array_slice($a, 1, 2)));       // [1,2]
printnl(json_encode(array_slice($a, -2)));         // [10,5]
printnl(json_encode(array_merge([1, 2], [3, 4]))); // [1,2,3,4]
printnl(json_encode(array_unique([1, 2, 2, 3, 1])));// [1,2,3]
printnl(in_array($a, 10), " ", in_array($a, 99));  // true false
printnl("done");
