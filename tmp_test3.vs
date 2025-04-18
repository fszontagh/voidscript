object $child1 = { name: "Child1", age: 10 };
object $child2 = { name: "Child2", age: 15 };
object $family = { $children: $child1, object $children: $child2 };
printnl($family->children->name);
