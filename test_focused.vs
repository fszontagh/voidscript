// Focus test for for-in loop scope issue

function testForLoop(string[] $items) {
    printnl("Function param length: ", array_length($items));
    
    // This should work
    printnl("About to start for loop...");
    
    for (string $item : $items) {
        printnl("In loop, item: ", $item);
        printnl("In loop, trying to access param: ", array_length($items));
        break;
    }
    
    printnl("After loop");
}

string[] $data = ["test"];
testForLoop($data);
