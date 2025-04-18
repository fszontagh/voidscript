object $child = { name: "Test", age: 5 }; object $parent = { child: $child }; printnl("Name: ",$parent->child->name, " Age: ", $parent->child->age);
