 # VariableTypes

 Provides an overview of the built-in variable types supported by VoidScript.

 ## Types

 ### int

 - **Description:** Integer type (whole numbers).
 - **Declaration:** `int $x = 42;`
 - **Literal values:** Numeric literals without a decimal point (e.g., `42`).

 ### double

 - **Description:** Double-precision floating point type.
 - **Declaration:** `double $pi = 3.14159;`
 - **Literal values:** Numeric literals with a decimal point (e.g., `3.14`).

 ### float

 - **Description:** Single-precision floating point type.
 - **Declaration:** `float $f = 1.23;`
 - **Literal values:** Numeric literals with a decimal point (e.g., `1.23`).

 ### string

 - **Description:** String type (text).
 - **Declaration:** `string $s = "hello";`
 - **Literal values:** String literals enclosed in quotes (e.g., `"hello"`).

 ### bool

 - **Description:** Boolean type.
 - **Declaration:** `bool $flag = true;`
 - **Literal values:** `true` or `false`.

 ### object

 - **Description:** Generic object (map) type for key-value pairs, used for objects and associative arrays.
 - **Declaration:** `object $o = {};`
 - **Usage:** Access fields via `$o["key"] = value;`.

 ### null

 - **Description:** Null type, can not declare variables with this type
 - **Declaration:** none
 - **Literal values:** `null`
 - **Usage:** Represents absence of a value

 ### undefined

 - **Description:** Undefined type.
 - **Usage:** Default type of a value when uninitialized or absent.

 ### User-defined classes

 - **Description:** Class instance types created from user-defined classes.
 - **Declaration:** After `class Person { ... }`, declare with `Person $p = new Person();`.
 - **Literal values:** Created via the `new` keyword.

 ## Example

```vs
int $a = 10; 

double $pi = 3.1415; 

float $f = -0.5; 

string $name = "VoidScript"; 

bool $ok = true; 

object $data = { int id = 0 }; 
$data->id = 1; 


class Person { 
    private: 
        string $name; 

    public: 
        constructor(string $name) { 
            $this->name = $name; 
        } 
}
Person $p = new Person("Alice");

// Inspect types
printnl(typeof($a));        // int
printnl(typeof($pi));       // double
printnl(typeof($f));        // float
printnl(typeof($name));     // string
printnl(typeof($ok));       // bool
printnl(typeof($data));     // object
printnl(typeof($p));        // class
```