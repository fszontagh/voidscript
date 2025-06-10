// Test array operations
int[] $numbers = [1, 2, 3, 4, 5];
string[] $fruits = ["apple", "banana", "cherry"];

printnl("Numbers array:");
for (int $num : $numbers) {
    printnl("  ", $num);
}

printnl("Fruits array:");
for (string $fruit : $fruits) {
    printnl("  ", $fruit);
}

// Array indexing
printnl("First number: ", $numbers[0]);
printnl("Second fruit: ", $fruits[1]);

// Array modification
$numbers[0] = 10;
$fruits[2] = "grape";

printnl("Modified first number: ", $numbers[0]);
printnl("Modified third fruit: ", $fruits[2]);

// Array length (if supported)
printnl("Numbers array has ", count($numbers), " elements");
printnl("Fruits array has ", count($fruits), " elements");