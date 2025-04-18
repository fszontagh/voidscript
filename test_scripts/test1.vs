string $name = "World 😀"; # world test
string $greeting = "Hello ";
string $smiley = "😀 = \\U0001F600 = \U0001F600\n";
int $number = 123;
double $number2 = 12.3;


print("The number: ", $number, "\n");
print("The number2: ", $number2, "\n");

print("Unicode: \u00E9 \U0001F600, hex: \x41, newline:\nEnd\t",$greeting, $name, "\n\nSmiley test: ", $smiley);
