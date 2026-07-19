
int $i = 0;
int $var = 0;

printnl("Starting while: ", $i);
while ($i < 45) {
    $var = $i;
    printnl("$i: ", $i);
    $i++;
}

printnl("Finished while: ", $i, " var: ", $var);