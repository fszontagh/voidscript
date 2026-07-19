printnl("Starting while loop");

int $i = 0;

int $var = 0;
while ($i < 10) {
    $var = $i;
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
int $var2 = 0;
for (int $key, auto $val : $obj) {
    $var2 = $val;
    printnl("key: ", $key, " val: ", $val);
}
printnl("var2 from for-in: ", $var2);

// C-style for loop
int $var3 = 0;
for (int $j = 0; $j < 2; $j++) {
    $var3 = $j;
    printnl("j: ", $j);
}
printnl("var3 from c-style for: ", $var3);