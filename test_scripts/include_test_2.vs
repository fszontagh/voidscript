// Second included file
// This file attempts to redefine variables from the main file
// This should cause a symbol conflict error

string $var = "attempting to redefine main file variable";  // This should fail
int $k = 200;  // This is fine since it's a new variable 