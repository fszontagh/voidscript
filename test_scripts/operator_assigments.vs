int $i = 0;
$i += 10;
$i -= 1;
$i /= 1;
printnl($i);


int $x = 5;
$x %= 2;
printnl($x);

int $u =  0;


printnl("$u = ", $u);
$u++;
printnl("$u = ", $u);

$i = 0;
int $l = 0;
for (int $g = 0;$g <= 10; $g++) {
    printnl("$g: ", $g);
    printnl("$i: ", $i);
    printnl("$l: ", $l);
    $i++;
    ++$l;
    printnl("---");
}
printnl("$i: ", $i);
printnl("$l: ", $l);
printnl("$g: ", $g);