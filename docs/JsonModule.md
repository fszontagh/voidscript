# JsonModule

Provides JSON serialization and deserialization functions.

## Functions

### json_encode

* **Signature:** `json_encode(value) -> string`

* **Description:** Serializes a VoidScript value to a JSON string.

* **Parameters:**

  * `value`: A VoidScript value (int, double, bool, string, object, null).

* **Returns:** JSON string representation.

* **Errors:**

  * Incorrect number of arguments.

### json_decode

* **Signature:** `json_decode(json) -> object|value`

* **Description:** Parses a JSON string into a VoidScript value.

* **Parameters:**

  * `json` (string): A valid JSON string.

* **Returns:** VoidScript value (object, number, bool, null).

* **Errors:**

  * Incorrect number of arguments.

  * `json` not a string.

  * Invalid JSON format.

* **Note:** Only JSON objects (`{}`), primitives, and null are supported; arrays (`[]`) are not supported.

## Example

    

`# JSON Encode/Decode Feature Test`\
`# Define an object with nested data`\
`object $user = {`\
`    string name: "Alice",`\
`    int age: 30,`\
`    boolean active: true,`\
`    object prefs: {`\
`        string theme: "dark",`\
`        boolean notifications: false`\
`    }`\
`};`

`// Encode to JSON string`\
`string $json = json_encode($user);`\
`printnl("Encoded JSON: ", $json);`