
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
