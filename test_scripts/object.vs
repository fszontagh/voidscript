
object $person = {
    name: "Szoni",
    double age: 37.6
};


object $person2 = {
    string name: "Not Szoni",
    int age: 37,
    object $children: {
        string name: "Child1",
        int age: 10
    }
};

object $children2 = {
     string name: "Child2",
        int age: 15
};

object $family = {
           $children: $children, // this is valid too
    object $children: $children2
};

object $family3 = {
    string age: 10  // this is invalid, drops error
};



printnl("Hello, ", $person->name);              // prints out: Hello, Szoni
println("Type: ", typeof($person);              // prints out: Type: object
println("Type: ", typeof($person2->name));      // prints out: Type: string
println("Type: ", typeof($person2->age));       // prints out: Type: double
