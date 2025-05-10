const string $world = "World";
printnl("Hello ", $world, "!");

const string $include1 = "this is from include 1";


const int $a1 = 10;
const int $b1 = 1;

include "include_2.vs";


printnl("intvar: ", $intvar);


const int $c = sum($a1, $b1);

printnl("Sum of ", $a1, " + ", $b1, " = ", $c);

include "include_3.vs";

Person $batman = new Person("Batman", 43);
printnl("Name: ", $batman->getName(), " Age: ", $batman->getAge());