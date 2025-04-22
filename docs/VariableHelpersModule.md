 # VariableHelpersModule

 Provides helper functions to inspect variable types.

 ## Functions

 ### typeof

 - **Signature:** `typeof(value) -> string`
 - **Description:** Returns the type name of `value`: `"int"`, `"double"`, `"float"`, `"string"`, `"bool"`, `"object"`, `"null"`, or `"undefined"`.
 - **Parameters:**
   - `value`: Any VoidScript value.
 - **Returns:** Type name as string.
 - **Errors:** Incorrect number of arguments.

 - **Signature:** `typeof(value, typeName) -> bool`
 - **Description:** Checks if the type of `value` matches `typeName`.
 - **Parameters:**
   - `value`: Any VoidScript value.
   - `typeName` (string): The type name to compare.
 - **Returns:** Boolean indicating if types match.
 - **Errors:**
   - Incorrect number of arguments.
   - `typeName` not a string.

 ## Example

 ```vs
 int $x = 42;
 printnl(typeof($x));              // int
 printnl(typeof($x, "int"));     // true
 printnl(typeof($x, "string"));  // false

 string $s = "hello";
 printnl(typeof($s));              // string
 printnl(typeof($s, "bool"));    // false
 ```