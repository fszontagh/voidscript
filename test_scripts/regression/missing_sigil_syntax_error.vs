// Regression: bug #1 - parser must not hang on a declaration missing the $ sigil.
// `int y = 5;` is invalid VoidScript (all variables require the $ sigil).
// The parser used to loop forever here because parseTopLevelStatement() returned
// without consuming a token, so the top-level loop never advanced.
// Expected: a syntax error on stderr and a non-zero exit, promptly.
int y = 5;
printnl($y);
