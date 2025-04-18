
object $person = {
    string name: "Szoni",
    int age: 37
};

printnl("Hello, ", $person->name, " your age is: ", $person->age);


object $person2 = {
    string name: "Not Szoni",
    int age: 37,
    object children: {
        string name: "Child1",
        int age: 10
    }
};
printnl("Person2: ", $person2->name, " age: ", $person2->age, " child: ", $person2->children->name, " age: ", $person2->children->age);

object $test = $person2;
printnl("Person2: ", $test->name, " age: ", $test->age, " child: ", $test->children->name, " age: ", $test->children->age);

string $person_name = $person->name;
printnl("Person name: ", $person_name);


printnl("Child1 old age: ",$person2->children->age);
$person2->children->age = $person2->children->age + 2;
printnl("Child1 new age: ",$person2->children->age);

int $age = 10;
if ($person2->children->age > 18) {
    printnl("Child1 is old enough to go to school.");
} else {
    printnl("Child1 is too young to go to school.");
}

for (string $key, auto $value : $person2) {

    if (typeof($value,"object") == false) {
        printnl("Key: ", $key, " Value: ", $value);
    }else {
        printnl("Key: ", $key, " is an object");
    }
}