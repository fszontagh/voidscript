const string $world = "World";
printnl("Hello ", $world, "!");

const string $include1 = "this is from include 1";




include "/home/fszontagh/soniscript/test_scripts/include_2.vs";


printnl("intvar: ", $intvar);

int $i = 0;
while ($i < 10) {
    printnl("$i: ", $i);
    $i++;
}