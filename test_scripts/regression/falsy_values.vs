// Regression: bug #19 - the ValuePtr truthiness trap (see #18) struck several more
// call sites. `if (!value)` on a ValuePtr does NOT test pointer validity:
// ValuePtr::operator bool() returns the VALUE's truthiness, and it *throws* on a
// genuinely null value rather than returning false. So `if (!value)` is wrong in
// both directions.
//
// Consequences this covers:
//   json_encode(0)   -> "JSON encoding failed: Cannot convert null ValuePtr to JSON"
//   json_encode("")  -> same
//   var_dump(0)      -> printed "NULL"
//   var_dump("")     -> printed "NULL"
//
// Expected: clean exit 0.

// --- json_encode on falsy scalars ---
printnl(json_encode(0));            // 0
printnl(json_encode(""));           // ""
printnl(json_encode(false));        // false

// non-falsy scalars must keep working
printnl(json_encode(42));           // 42
printnl(json_encode("hi"));         // "hi"
printnl(json_encode(true));         // true

// falsy values nested in a container
object $o = { int n: 0, string s: "", boolean b: false };
printnl(json_encode($o));           // {"b":false,"n":0,"s":""}

// --- round trip through decode ---
object $rt = json_decode("{\"n\":0,\"s\":\"\",\"b\":false}");
printnl($rt->n);                    // 0
printnl($rt->s);                    // (empty line)
printnl($rt->b);                    // false

// --- var_dump must not call falsy values NULL ---
string $d0 = var_dump(0);
string $ds = var_dump("");
printnl(string_length(string_replace($d0, "NULL", "")) == string_length($d0));  // true
printnl(string_length(string_replace($ds, "NULL", "")) == string_length($ds));  // true

printnl("done");
