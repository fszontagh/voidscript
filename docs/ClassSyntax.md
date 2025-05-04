# VoidScript Class Syntax Guide

This document explains how to use classes in VoidScript.

## Class Definition

A class is defined using the `class` keyword followed by the class name and braces:

```
class MyClass {
    // Properties and methods go here
}
```

## Properties

Properties are defined with their type and name:

```
class Person {
    public: string $name = "Default";  // With default value
    private: int $age;                 // Without default value
}
```

Access modifiers:
- `public:` - Can be accessed outside the class
- `private:` - Can only be accessed within the class

## Constructors

Constructors are defined using the `construct` function:

```
class Person {
    private: string $name;
    private: int $age;
    
    function construct(string $name, int $age) {
        this->$name = $name;
        this->$age = $age;
    }
}
```

## Methods

Methods are defined like functions but inside the class:

```
class Person {
    private: string $name;
    
    function getName() string {
        return this->$name;
    }
    
    function setName(string $newName) {
        this->$name = $newName;
    }
}
```

## Accessing Properties

- Inside class methods, use `this->$propertyName`
- For public properties, use `$objectName->$propertyName`

## Class Instantiation

Create a new instance using the `new` keyword:

```
Person $person = new Person("John", 30);
```

## Example

```
class Person {
    private: string $name;
             int $age;
    
    function construct(string $name, int $age) {
        this->$name = $name;
        this->$age = $age;
    }
    
    function getName() string {
        return this->$name;
    }
    
    function getAge() int {
        return this->$age;
    }
    
    function isAdult() bool {
        return this->$age >= 18;
    }
}

// Create an instance
Person $person = new Person("John", 25);

// Use methods
printnl("Name: ", $person->$getName());
printnl("Age: ", $person->$getAge());

if ($person->$isAdult()) {
    printnl($person->$getName(), " is an adult");
} else {
    printnl($person->$getName(), " is not an adult");
}
```

## Important Notes

1. Always use `this->$property` syntax inside methods to access class properties.
2. Class names cannot be reserved words like `function`, `string`, `null`, `int`, etc.
3. Public properties can be accessed directly with `$object->$property`.
4. Private properties can only be accessed within the class using `this->$property`. 