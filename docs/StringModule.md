 # StringModule

 Provides helper functions for string manipulation.

 ## Functions

 ### string_length

 - **Signature:** `string_length(str) -> int`
 - **Description:** Returns the length of the string `str`.
 - **Parameters:**
   - `str` (string): The input string.
 - **Returns:** Integer length.
 - **Errors:**
   - Incorrect number of arguments.
   - `str` not a string.

 ### string_replace

 - **Signature:** `string_replace(str, from, to, replace_all) -> string`
 - **Description:** Replaces occurrences of substring `from` with `to` in `str`.
 - **Parameters:**
   - `str` (string): The input string.
   - `from` (string): Substring to replace.
   - `to` (string): Replacement substring.
   - `replace_all` (bool): `true` to replace all occurrences, `false` to replace only the first.
 - **Returns:** New string with replacements.
 - **Errors:**
   - Incorrect number of arguments.
   - Wrong argument types.
   - `from` is empty.

 ### string_substr

 - **Signature:** `string_substr(str, from, length) -> string`
 - **Description:** Extracts a substring from `str`.
 - **Parameters:**
   - `str` (string): The input string.
   - `from` (int): Starting index (0-based).
   - `length` (int): Number of characters to extract.
 - **Returns:** The substring.
 - **Errors:**
   - Incorrect number of arguments.
   - Wrong argument types.
   - `from` or `length` negative or out of range.

 ## Example

 ```vs
 var s = "Hello, world!";
 printnl(string_length(s));  // 13
 printnl(string_replace(s, "world", "VoidScript", false));  // Hello, VoidScript!
 printnl(string_substr(s, 7, 5));  // world
 ```