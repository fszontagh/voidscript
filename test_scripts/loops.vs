printnl("Starting while loop");

int $i = 0;

while ($i < 10) {
    int $var = $i;
    printnl("i: ", $i);
    $i++;
}

printnl("var from while: ", $var);
printnl("Finished while loop");

int $z = 2;
for (int $i2 = 0; $i2 < 10; $i2++) {
    printnl("i2: ", $i2);
    $z++;
}

printnl("z: ", $z);
printnl("Finished for loop");

for (int $i3 = 0; $i3 < 10; $i3++) {
    printnl("i3: ", $i3);
}

printnl("Finished for loop");

object $obj = {
    int $a : 1,
    int $b : 2
};

// For-in loop
for (int $key, auto $val : $obj) {
    int $var2 = $val;
    printnl("key: ", $key, " val: ", $val);
}
printnl("var2 from for-in: ", $var2);

// C-style for loop
for (int $j = 0; $j < 2; $j++) {
    int $var3 = $j;
    printnl("j: ", $j);
}
printnl("var3 from c-style for: ", $var3);