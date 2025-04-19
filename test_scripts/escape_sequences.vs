# String Escape Sequences Feature Test

// Newline
string $new = "Hello\nWorld";
print("$new ->\n", $new, "\n");

// Tab
string $tab = "col1\tcol2";
print("$tab -> [", $tab, "]\n");

// Double-quote
string $dq = "\"in quotes\"";
print("$dq -> [", $dq, "]\n");

// Unknown escape (\x treated as x)
string $ux = "test\xescape";
print("$ux -> [", $ux, "]\n");