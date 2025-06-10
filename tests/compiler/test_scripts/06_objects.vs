// Test object operations
object $person = {
    string $name : "John",
    int $age : 30,
    bool $active : true
};

printnl("Person name: ", $person->name);
printnl("Person age: ", $person->age);
printnl("Person active: ", $person->active);

// Modify object properties
$person->name = "Jane";
$person->age = 25;
$person->active = false;

printnl("Updated name: ", $person->name);
printnl("Updated age: ", $person->age);
printnl("Updated active: ", $person->active);

// Object iteration
printnl("Object properties:");
for (string $key, auto $value : $person) {
    printnl("  ", $key, ": ", $value);
}