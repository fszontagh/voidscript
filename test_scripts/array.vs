// Comprehensive array example with supported patterns
// Arrays are best accessed with for-each loops in this language

// 1. Creating arrays
printnl("==== Creating Arrays ====");
int[] $int_array = [10, 20, 30, 40, 50];
printnl("Integer array size: ", sizeof($int_array));

string[] $string_array = ["apple", "banana", "cherry"];
printnl("String array size: ", sizeof($string_array));

// 2. Iterating through arrays (best practice)
printnl("\n==== Array Iteration with for-each ====");
printnl("Integer array values:");
for (int $value : $int_array) {
    printnl("Value: ", $value);
}

printnl("\nString array values:");
for (string $str : $string_array) {
    printnl("String: ", $str);
}

// 3. Iterating with index (if you need position)
printnl("\n==== Array Iteration with index ====");
for (int $i = 0; $i < sizeof($int_array); $i++) {
    printnl("Position ", $i);
}

// 4. Finding values via iteration
printnl("\n==== Finding Values in Arrays ====");
int $search_value = 30;
printnl("Searching for value: ", $search_value);

bool $found = false;
for (int $value : $int_array) {
    if ($value == $search_value) {
        $found = true;
    }
}

if ($found) {
    printnl("Value found in array!");
} else {
    printnl("Value not found.");
}

// 5. Collecting values from arrays
printnl("\n==== Working with Array Values ====");
int $sum = 0;
for (int $value : $int_array) {
    $sum = $sum + $value;
}
printnl("Sum of all values: ", $sum);

// 6. Modifying arrays
printnl("\n==== Modifying Arrays ====");
// Assign a new array
$int_array = [100, 200, 300];
printnl("New array size: ", sizeof($int_array));
printnl("New array values:");
for (int $value : $int_array) {
    printnl("Value: ", $value);
}

// 7. NULL handling
printnl("\n==== NULL Array Handling ====");
int[] $null_array = NULL;

if ($null_array == NULL) {
    printnl("Array is NULL");
} else {
    printnl("Array is not NULL");
}

// Initialize from NULL
$null_array = [1, 2, 3];
printnl("Array size after initialization: ", sizeof($null_array));
printnl("Array values:");
for (int $value : $null_array) {
    printnl("Value: ", $value);
} 