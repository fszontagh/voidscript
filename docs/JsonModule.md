 # JsonModule

 Provides JSON serialization and deserialization functions.

 ## Functions

 ### json_encode

 - **Signature:** `json_encode(value) -> string`
 - **Description:** Serializes a VoidScript value to a JSON string.
 - **Parameters:**
   - `value`: A VoidScript value (int, double, bool, string, object, null).
 - **Returns:** JSON string representation.
 - **Errors:**
   - Incorrect number of arguments.

 ### json_decode

 - **Signature:** `json_decode(json) -> object|value`
 - **Description:** Parses a JSON string into a VoidScript value.
 - **Parameters:**
   - `json` (string): A valid JSON string.
 - **Returns:** VoidScript value (object, number, bool, null).
 - **Errors:**
   - Incorrect number of arguments.
   - `json` not a string.
   - Invalid JSON format.
 - **Note:** Only JSON objects (`{}`), primitives, and null are supported; arrays (`[]`) are not supported.

 ## Example

 ```vs
 var obj = {};
 obj["name"] = "Alice";
 obj["age"] = 30;
 var jsonStr = json_encode(obj);
 printnl(jsonStr);  // {"name":"Alice","age":30}

 var decoded = json_decode(jsonStr);
 printnl(decoded["name"]);  // Alice
 ```