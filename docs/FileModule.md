 # FileModule

 Provides simple file I/O functions.

 ## Functions

 ### file_get_contents

 - **Signature:** `file_get_contents(filename) -> string`
 - **Description:** Reads entire content of a file.
 - **Parameters:**
   - `filename` (string): Path to the file.
 - **Returns:** File content as a string.
 - **Errors:**
   - Incorrect number of arguments.
   - `filename` not a string.
   - File does not exist or cannot be opened.

 ### file_put_contents

 - **Signature:** `file_put_contents(filename, content, overwrite) -> undefined`
 - **Description:** Writes `content` to `filename`.
 - **Parameters:**
   - `filename` (string): Path to the file.
   - `content` (string): Content to write.
   - `overwrite` (bool): Whether to overwrite existing file.
 - **Returns:** VoidScript `undefined`.
 - **Errors:**
   - Incorrect number of arguments.
   - Wrong types of arguments.
   - File exists when `overwrite` is false.
   - Unable to open or write to file.

 ### file_exists

 - **Signature:** `file_exists(filename) -> bool`
 - **Description:** Checks if a file exists.
 - **Parameters:**
   - `filename` (string): Path to the file.
 - **Returns:** Boolean (true if exists, false otherwise).
 - **Errors:**
   - Incorrect number of arguments.
   - `filename` not a string.

 ## Example

 ```vs
 var filename = "example.txt";
 if (!file_exists(filename)) {
     file_put_contents(filename, "Hello, VoidScript!", true);
 }
 var content = file_get_contents(filename);
 printnl(content);
 ```