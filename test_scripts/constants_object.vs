# Constants Object Feature Test
# Test declaration of immutable object constants and verify property modification errors

# Declare constant object and print its properties
const object $person = {
    string name: "Bruce Wayne",
    int age: 42,
    object address: {
        string city: "Gotham",
        int zip: 12345
    }
};
printnl($person->name, " is ", $person->age, " years old.");
printnl("City: ", $person->address->city, ", ZIP: ", $person->address->zip);

# Attempt to modify a property of the constant object (should produce runtime error)
// $person->age = 43;