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

 ### mkdir

 - **Signature:** `mkdir(path, recursive) -> bool`
 - **Description:** Creates a directory at the specified path.
 - **Parameters:**
   - `path` (string): Path to the directory to create.
   - `recursive` (bool, optional): Whether to create parent directories if they don't exist. Defaults to false.
 - **Returns:** Boolean (true if directory was created successfully, false otherwise).
 - **Errors:**
   - Incorrect number of arguments.
   - `path` not a string.
   - `recursive` not a boolean (when provided).
   - Directory already exists.
   - Parent directory doesn't exist (when recursive is false).

 ### rmdir

 - **Signature:** `rmdir(path) -> bool`
 - **Description:** Removes an empty directory at the specified path.
 - **Parameters:**
   - `path` (string): Path to the directory to remove.
 - **Returns:** Boolean (true if directory was removed successfully, false otherwise).
 - **Errors:**
   - Incorrect number of arguments.
   - `path` not a string.
   - Directory does not exist.
   - Directory is not empty.
   - Permission denied.

 ## Example

 ```vs
 // File operations
 var filename = "example.txt";
 if (!file_exists(filename)) {
     file_put_contents(filename, "Hello, VoidScript!", true);
 }
 var content = file_get_contents(filename);
 printnl(content);

 // Directory operations
 var dirname = "test_directory";
 
 // Create a simple directory
 if (mkdir(dirname)) {
     printnl("Directory created successfully");
 }
 
 // Create nested directories recursively
 var nested_path = "parent/child/grandchild";
 if (mkdir(nested_path, true)) {
     printnl("Nested directories created successfully");
 }
 
 // Remove empty directories
 if (rmdir(dirname)) {
     printnl("Directory removed successfully");
 }
 
 // Clean up nested directories (must be done in reverse order)
 rmdir("parent/child/grandchild");
 rmdir("parent/child");
 rmdir("parent");
 ```