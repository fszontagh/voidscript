 # PrintModule

 Provides printing functions to standard output and error.

 ## Functions

 ### print

 - **Signature:** `print(...values) -> undefined`
 - **Description:** Prints the string representation of each value without a newline.
 - **Parameters:** Any number of values.
 - **Returns:** undefined.

 ### printnl

 - **Signature:** `printnl(...values) -> undefined`
 - **Description:** Prints the string representation of each value followed by a newline.
 - **Parameters:** Any number of values.
 - **Returns:** undefined.

 ### error

 - **Signature:** `error(...values) -> undefined`
 - **Description:** Prints the string representation of each value to standard error followed by a newline.
 - **Parameters:** Any number of values.
 - **Returns:** undefined.

 ## Example

 ```vs
 print("Hello, ");
 printnl("world!");  // prints "Hello, world!" with newline
 error("An error occurred");  // prints to stderr
 ```