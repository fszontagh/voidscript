#!/usr/bin/env voidscript

// Test var_dump function with different variable types

// Test primitive types
string $str = "Hello World";
printnl("String test:");
printnl(var_dump($str));

int $num = 42;
printnl("Integer test:");
printnl(var_dump($num));

double $float_val = 3.14159;
printnl("Float test:");
printnl(var_dump($float_val));

bool $bool_val = true;
printnl("Boolean test:");
printnl(var_dump($bool_val));

// Test object
object $obj = {
    string name: "John",
    int age: 30,
    bool active: true
};
printnl("Object test:");
printnl(var_dump($obj));

// Test nested object
object $nested = {
    object user: {
        object profile: {
            string name: "Jane",
            string email: "jane@example.com"
        },
        object settings: {
            string theme: "dark",
            bool notifications: false
        }
    },
    object data: {
        string first: "value1",
        string second: "value2", 
        string third: "value3"
    }
};
printnl("Nested object test:");
printnl(var_dump($nested));

// Test array-like object (using unquoted keys)
object $array_like = {
    string first: "apple",
    string second: "banana",
    string third: "cherry"
};
printnl("Array-like object test:");
printnl(var_dump($array_like));

printnl("var_dump test completed!");
