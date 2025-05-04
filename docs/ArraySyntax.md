# VoidScript Array Syntax Guide

This document explains how to use arrays in VoidScript.

## Array Declaration

Arrays can be declared with the type followed by square brackets:

```
int[] $numbers = [1, 2, 3, 4, 5];
string[] $names = ["Alice", "Bob", "Charlie"];
```

## Iterating Through Arrays

The best way to iterate through arrays is with a for-each loop:

```
// For-each loop
for (int $value : $numbers) {
    printnl($value);
}

// For-each with index
for (int $i, string $name : $names) {
    printnl("Index ", $i, ": ", $name);
}
```

## Array Operations

```
// Creating an array
string[] $fruits = ["apple", "banana", "orange"];

// Accessing array elements via for loop
for (int $i, string $fruit : $fruits) {
    printnl("Fruit at index ", $i, ": ", $fruit);
}

// Creating an empty array and filling it
int[] $scores = [];
for (int $i = 0; $i < 5; $i++) {
    $scores[] = $i * 10;
}

// Getting array length
int $length = $scores->length();
printnl("Array length: ", $length);

// Using array methods
$fruits->push("grape");  // Add element to the end
$fruits->pop();          // Remove last element
$fruits->shift();        // Remove first element
$fruits->unshift("kiwi"); // Add element at beginning
```

## Multi-dimensional Arrays

```
// 2D array
int[][] $matrix = [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
];

// Accessing elements in a 2D array
for (int[] $row : $matrix) {
    for (int $cell : $row) {
        printnl($cell);
    }
}
```

## Array Methods

VoidScript arrays support several built-in methods:

- `push(item)` - Add an item to the end
- `pop()` - Remove and return the last item
- `shift()` - Remove and return the first item
- `unshift(item)` - Add an item to the beginning
- `length()` - Get the array length
- `join(separator)` - Join array elements into a string
- `slice(start, end)` - Get a portion of the array
- `reverse()` - Reverse the array
- `sort()` - Sort the array

## Example

```
// Create an array
string[] $colors = ["red", "green", "blue"];

// Add elements
$colors->push("yellow");
$colors->unshift("purple");

// Iterate through elements
for (int $i, string $color : $colors) {
    printnl("Color ", $i, ": ", $color);
}

// Get joined string
string $colorString = $colors->join(", ");
printnl("Colors: ", $colorString);

// Sort the array
$colors->sort();
printnl("Sorted colors: ", $colors->join(", "));
```

## Current Limitations

1. Direct index access (`$array[0]`) may not work in all contexts; prefer using for-each loops
2. Some array operations might have limited support in the current parser version 