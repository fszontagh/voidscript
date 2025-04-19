 # ArrayModule

 Provides the `sizeof` function to get the number of elements in an array (object map).

 ## Functions

 ### sizeof

 - **Signature:** `sizeof(array) -> int`
 - **Description:** Returns the number of elements in `array`.
 - **Parameters:**
   - `array` (object): The array (object map) whose elements to count.
 - **Returns:** Integer count of elements.
 - **Errors:**
   - Incorrect number of arguments.
   - Argument is not an array (object).

 ## Example

 ```vs
 var arr = {};
 arr["first"] = 10;
 arr["second"] = 20;
 printnl("Length:", sizeof(arr));  // Length: 2
 ```