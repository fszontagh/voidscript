object $person = {
    string name: "Batman",
    int age: 37
};

printnl("Hello, ", $person->name);  // Just test a simple property access


object $person2 = {
    string name: "Not Batman",
    int age: 37,
    object children: {
        string name: "Child1",
        int age: 10
    }
};
printnl("Person2: ", $person2->name, " age: ", $person2->age, " child: ", $person2->children->name, " age: ", $person2->children->age);

object $test        = $person2;
printnl("Person2: ", $test->name, " age: ", $test->age, " child: ", $test->children->name, " age: ", $test->children->age);

string $person_name = $person->name;
printnl("Person name: ", $person_name);


printnl("Child1 old age: ", $person2->children->age);
int $new_age = 20;
$person2->children->age = $new_age;
printnl("Child1 new age: ", $person2->children->age);

// Direct assignment of numeric literal - should work now after parser changes
$person2->children->age = 21;
printnl("Child1 final age: ", $person2->children->age);

int $another_age = 11;
$person2->children->age = $another_age;
int $age = 10;
int $adult_age = 18;

if ($person2->children->age > $adult_age) {
    printnl("Child1 is old enough to go to school.");
} else {
    printnl("Child1 is too young to go to school.");
}

int $third_age = 21;
$person2->children->age = $third_age;

for (string $key, auto $value : $person2) {

    if (typeof($value,"object") == false) {
        printnl("Key: ", $key, " Value: ", $value);
    }else {
        printnl("Key: ", $key, " is an object");
    }
}
$person2->children->age = 33;

printnl("Child1 age: ", $person2->children->age);