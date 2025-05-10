string $name = "World";
if ($argc == 2) {
    $name = $argv[1];
}
print("Hello, ", $name);
