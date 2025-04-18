string $test = "Test string";


if ($test == "Test string") {
    printnl("Test passed");
} else {
    printnl("Test failed");
}


bool $this_is_okay = true;
bool $this_is_not_okay = false;


if ($this_is_okay && $this_is_not_okay == false && $test == "Test string") {
    printnl("This is okay");
}

if ($this_is_okay == false) {
    printnl("This is not okay");
}