// Regression: bug #8 - string literals were opaque to the lexer, so "Hello $n"
// printed literally. Double-quoted strings now interpolate $variables, as in PHP.
// Single-quoted strings do not, which is how you opt out.
// Expected: clean exit 0.

string $name = "world";
int $n = 42;
double $d = 1.5;

printnl("Hello $name");                 // Hello world
printnl("n=$n d=$d");                   // n=42 d=1.500000
printnl("$name$name");                  // worldworld
printnl("[$name]");                     // [world]

// braced form, so the name can be delimited explicitly
printnl("${name}s");                    // worlds

// escaping: a literal dollar
printnl("costs \$5");                   // costs $5

// a lone $ or one followed by a non-identifier stays literal
printnl("100% $ ok");                   // 100% $ ok

// single quotes never interpolate
printnl('raw $name');                   // raw $name

// object members interpolate through the braced form
object $o = {};
$o->field = "inner";
printnl("got ${o->field}");             // got inner

// interpolation is not applied twice
string $tricky = "$name";
printnl("$tricky");                     // world

printnl("done");
