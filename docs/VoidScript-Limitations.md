# VoidScript Current Limitations

This document outlines some current limitations and issues in the VoidScript parser that developers should be aware of.

## Class Method Implementation

The current parser has issues with class method implementation, particularly:

1. The `$this->property` syntax in method bodies sometimes fails to parse correctly, especially in:
   - Constructor methods
   - Methods that access multiple class properties
   - Methods that perform operations on class properties

## Working Class Patterns

Until the parser is fixed, you can use these patterns that work consistently:

### Public Properties Without Methods

```
class Simple {
    public: string $name = "Default";
    public: int $value = 42;
}

Simple $obj = new Simple();
printnl($obj->name);  // Works fine
```

### Minimal Class With Properties

```
class MinimalClass {
    public: string $name = "Test";
    public: int $age = 30;
}

MinimalClass $mc = new MinimalClass();
$mc->name = "New Name";
printnl($mc->name, " is ", $mc->age, " years old");
```

## Workarounds

1. Use public properties instead of private properties with getters/setters
2. Keep class methods as simple as possible, avoiding complex operations on `$this->property`
3. For complex logic, create functions outside the class

## Future Improvements

The VoidScript team is working on improving the parser to properly handle:

1. Full support for class methods with proper `$this->property` syntax
2. Better error messages for class-related syntax errors
3. More robust handling of return statements in class methods 