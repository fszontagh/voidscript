#!/usr/bin/env voidscript
printnl("=== VoidScript Documentation Generator ===");

// Check if directory argument is provided
// For now, we'll use a default directory since VoidScript may not have command line args access
// The script should be called with: ./voidscript assets/docs/generate_docs.vs
// and it will generate docs in the current directory or a subdirectory

string $docs_dir = "generated_docs";
printnl("Generating documentation in directory: ", $docs_dir);

// Create main documentation directory
if (!file_exists($docs_dir)) {
    printnl("Creating documentation directory: ", $docs_dir);
    // VoidScript may not have mkdir function, so we'll create a marker file
    file_put_contents($docs_dir + "/.docs_dir", "Documentation directory", true);
}

// Create subdirectories for builtin and dynamic modules
string $builtin_dir = $docs_dir + "/builtin_modules";
string $dynamic_dir = $docs_dir + "/dynamic_modules";

if (!file_exists($builtin_dir + "/.dir")) {
    file_put_contents($builtin_dir + "/.dir", "Built-in modules directory", true);
}

if (!file_exists($dynamic_dir + "/.dir")) {
    file_put_contents($dynamic_dir + "/.dir", "Dynamic modules directory", true);
}

printnl("Documentation directories created.");
printnl("Analyzing modules...");

// Get all modules
object $modules = module_list();
if (typeof($modules, "object") == false) {
    throw_error("module_list() did not return an object");
}

// Generate documentation for each module
for (string $module_key, object $module : $modules) {
    if (typeof($module, "object") == false) {
        throw_error("Invalid module object in list");
    }

    string $module_name = $module["name"];
    string $module_path = $module["path"];
    
    printnl("Processing module: ", $module_name);
    
    // Determine if this is a builtin or dynamic module
    string $target_dir = $builtin_dir;
    string $module_type = "Built-in";
    
    // Dynamic modules typically have specific paths or characteristics
    // For now, we'll classify based on available functions and classes
    object $module_classes = $module["classes"];
    object $module_functions = $module["functions"];
    
    // Check if this module has dynamic characteristics (classes like MariaDB, Xml2, etc.)
    bool $has_dynamic_classes = false;
    for (string $class_key, object $class_info : $module_classes) {
        string $class_name = $class_info["name"];
        if ($class_name == "MariaDB" || $class_name == "Xml2" || $class_name == "XmlNode" || 
            $class_name == "XmlAttr" || $class_name == "Imagick") {
            $has_dynamic_classes = true;
        }
    }
    
    // Check for dynamic functions (curl, format functions)
    bool $has_dynamic_functions = false;
    for (string $func_key, object $func_info : $module_functions) {
        string $func_name = $func_info["documentation"]["name"];
        if (string_substr($func_name, 0, 4) == "curl" || $func_name == "format" || $func_name == "format_print") {
            $has_dynamic_functions = true;
        }
    }
    
    if ($has_dynamic_classes || $has_dynamic_functions) {
        $target_dir = $dynamic_dir;
        $module_type = "Dynamic";
    }
    
    // Generate markdown content
    string $markdown_content = "";
    
    // Header
    $markdown_content = "# " + $module_name + " Module\n\n";
    $markdown_content = $markdown_content + "**Type:** " + $module_type + " Module  \n";
    $markdown_content = $markdown_content + "**Path:** `" + $module_path + "`  \n";
    $markdown_content = $markdown_content + "**Generated:** " + "2025-05-26" + "\n\n";
    
    // Overview
    $markdown_content = $markdown_content + "## Overview\n\n";
    if ($module_type == "Dynamic") {
        $markdown_content = $markdown_content + "This is a dynamic module that provides extended functionality through external libraries.\n\n";
    } else {
        $markdown_content = $markdown_content + "This is a built-in module that provides core VoidScript functionality.\n\n";
    }
    
    // Classes section
    if (sizeof($module_classes) > 0) {
        $markdown_content = $markdown_content + "## Classes\n\n";
        $markdown_content = $markdown_content + "This module provides " + sizeof($module_classes) + " class(es):\n\n";
        
        for (string $class_key, object $class_info : $module_classes) {
            string $class_name = $class_info["name"];
            $markdown_content = $markdown_content + "### " + $class_name + "\n\n";
            $markdown_content = $markdown_content + "**Class Name:** `" + $class_name + "`\n\n";
            
            // Usage example
            $markdown_content = $markdown_content + "**Usage:**\n```voidscript\n";
            $markdown_content = $markdown_content + "class " + $class_name + " $instance = new " + $class_name + "();\n";
            $markdown_content = $markdown_content + "```\n\n";
            
            // Class description based on name
            if ($class_name == "MariaDB") {
                $markdown_content = $markdown_content + "**Description:** Provides MariaDB/MySQL database connectivity and operations.\n\n";
            } else if ($class_name == "Xml2") {
                $markdown_content = $markdown_content + "**Description:** Main XML document handler for parsing and creating XML documents.\n\n";
            } else if ($class_name == "XmlNode") {
                $markdown_content = $markdown_content + "**Description:** Represents an XML element/node with content and attributes.\n\n";
            } else if ($class_name == "XmlAttr") {
                $markdown_content = $markdown_content + "**Description:** Represents an XML attribute with name and value.\n\n";
            } else if ($class_name == "Imagick") {
                $markdown_content = $markdown_content + "**Description:** Provides image manipulation and processing capabilities.\n\n";
            } else {
                $markdown_content = $markdown_content + "**Description:** " + $class_name + " class functionality.\n\n";
            }
        }
    }
    
    // Functions section
    if (sizeof($module_functions) > 0) {
        $markdown_content = $markdown_content + "## Functions\n\n";
        $markdown_content = $markdown_content + "This module provides " + sizeof($module_functions) + " function(s):\n\n";
        
        for (string $func_key, object $func_info : $module_functions) {
            object $func_doc = $func_info["documentation"];
            string $func_name = $func_doc["name"];
            
            $markdown_content = $markdown_content + "### " + $func_name + "()\n\n";
            
            // Function signature
            string $signature = $func_name + "(";
            object $parameters = $func_doc["parameters"];
            
            if (sizeof($parameters) > 0) {
                string $param_list = "";
                for (string $param_name, object $param_info : $parameters) {
                    if (string_length($param_list) > 0) {
                        $param_list = $param_list + ", ";
                    }
                    $param_list = $param_list + $param_info["type"] + " $" + $param_info["name"];
                }
                $signature = $signature + $param_list;
            }
            $signature = $signature + ")";
            
            $markdown_content = $markdown_content + "**Signature:** `" + $signature + "`\n\n";
            
            // Description
            if ($func_name == "curlGet") {
                $markdown_content = $markdown_content + "**Description:** Performs HTTP GET requests to specified URLs with optional configuration.\n\n";
            } else if ($func_name == "curlPost") {
                $markdown_content = $markdown_content + "**Description:** Performs HTTP POST requests with data payload to specified URLs.\n\n";
            } else if ($func_name == "curlPut") {
                $markdown_content = $markdown_content + "**Description:** Performs HTTP PUT requests with data payload to specified URLs.\n\n";
            } else if ($func_name == "curlDelete") {
                $markdown_content = $markdown_content + "**Description:** Performs HTTP DELETE requests to specified URLs.\n\n";
            } else if ($func_name == "format") {
                $markdown_content = $markdown_content + "**Description:** Formats strings using template syntax with {} placeholders.\n\n";
            } else if ($func_name == "format_print") {
                $markdown_content = $markdown_content + "**Description:** Formats and prints strings directly using template syntax.\n\n";
            } else if ($func_name == "module_list") {
                $markdown_content = $markdown_content + "**Description:** Returns a comprehensive list of all available modules, their classes, and functions.\n\n";
            } else if ($func_name == "module_exists") {
                $markdown_content = $markdown_content + "**Description:** Checks if a specified module exists in the system.\n\n";
            } else if ($func_name == "module_info") {
                $markdown_content = $markdown_content + "**Description:** Returns detailed information about a specific module.\n\n";
            } else if ($func_name == "function_doc") {
                $markdown_content = $markdown_content + "**Description:** Retrieves documentation for a specific function.\n\n";
            } else if ($func_name == "json_encode") {
                $markdown_content = $markdown_content + "**Description:** Converts VoidScript objects and arrays to JSON string format.\n\n";
            } else if ($func_name == "json_decode") {
                $markdown_content = $markdown_content + "**Description:** Parses JSON strings into VoidScript objects.\n\n";
            } else if ($func_name == "printnl") {
                $markdown_content = $markdown_content + "**Description:** Prints values to output with automatic newline.\n\n";
            } else if ($func_name == "print") {
                $markdown_content = $markdown_content + "**Description:** Prints values to output without automatic newline.\n\n";
            } else if ($func_name == "file_get_contents") {
                $markdown_content = $markdown_content + "**Description:** Reads the entire contents of a file as a string.\n\n";
            } else if ($func_name == "file_put_contents") {
                $markdown_content = $markdown_content + "**Description:** Writes string content to a file.\n\n";
            } else if ($func_name == "file_exists") {
                $markdown_content = $markdown_content + "**Description:** Checks if a file or directory exists.\n\n";
            } else if ($func_name == "file_size") {
                $markdown_content = $markdown_content + "**Description:** Returns the size of a file in bytes.\n\n";
            } else if ($func_name == "string_length") {
                $markdown_content = $markdown_content + "**Description:** Returns the length of a string.\n\n";
            } else if ($func_name == "string_substr") {
                $markdown_content = $markdown_content + "**Description:** Extracts a substring from a string.\n\n";
            } else if ($func_name == "string_replace") {
                $markdown_content = $markdown_content + "**Description:** Replaces occurrences of a substring with another string.\n\n";
            } else if ($func_name == "sizeof") {
                $markdown_content = $markdown_content + "**Description:** Returns the size/length of arrays and objects.\n\n";
            } else if ($func_name == "typeof") {
                $markdown_content = $markdown_content + "**Description:** Returns or checks the type of a variable.\n\n";
            } else if ($func_name == "error") {
                $markdown_content = $markdown_content + "**Description:** Displays error messages.\n\n";
            } else if ($func_name == "throw_error") {
                $markdown_content = $markdown_content + "**Description:** Throws a runtime error with a custom message.\n\n";
            } else {
                $markdown_content = $markdown_content + "**Description:** " + $func_name + " function.\n\n";
            }
            
            // Parameters
            if (sizeof($parameters) > 0) {
                $markdown_content = $markdown_content + "**Parameters:**\n\n";
                for (string $param_name, object $param_info : $parameters) {
                    $markdown_content = $markdown_content + "- `" + $param_info["name"] + "` (" + $param_info["type"] + "): " + $param_info["description"] + "\n";
                }
                $markdown_content = $markdown_content + "\n";
            }
            
            // Usage example
            $markdown_content = $markdown_content + "**Example:**\n```voidscript\n";
            if ($func_name == "curlGet") {
                $markdown_content = $markdown_content + "object $options = {};\n";
                $markdown_content = $markdown_content + "string $response = curlGet(\"https://api.example.com/data\", $options);\n";
            } else if ($func_name == "format") {
                $markdown_content = $markdown_content + "string $result = format(\"Hello {}, you have {} messages!\", \"John\", \"5\");\n";
            } else if ($func_name == "json_decode") {
                $markdown_content = $markdown_content + "object $data = json_decode(\"{\\\"name\\\": \\\"John\\\", \\\"age\\\": 30}\");\n";
            } else if ($func_name == "file_get_contents") {
                $markdown_content = $markdown_content + "string $content = file_get_contents(\"config.txt\");\n";
            } else if ($func_name == "printnl") {
                $markdown_content = $markdown_content + "printnl(\"Hello World!\");\n";
            } else if (sizeof($parameters) > 0) {
                $markdown_content = $markdown_content + "// Example usage of " + $func_name + "\n";
                string $example_call = $func_name + "(";
                string $example_params = "";
                for (string $param_name, object $param_info : $parameters) {
                    if (string_length($example_params) > 0) {
                        $example_params = $example_params + ", ";
                    }
                    if ($param_info["type"] == "string") {
                        $example_params = $example_params + "\"example\"";
                    } else if ($param_info["type"] == "int") {
                        $example_params = $example_params + "42";
                    } else if ($param_info["type"] == "bool") {
                        $example_params = $example_params + "true";
                    } else if ($param_info["type"] == "object") {
                        $example_params = $example_params + "{}";
                    } else {
                        $example_params = $example_params + "value";
                    }
                }
                $example_call = $example_call + $example_params + ");";
                $markdown_content = $markdown_content + $example_call + "\n";
            } else {
                $markdown_content = $markdown_content + $func_name + "();\n";
            }
            $markdown_content = $markdown_content + "```\n\n";
        }
    }
    
    // Variables section (if any)
    object $module_variables = $module["variables"];
    if (sizeof($module_variables) > 0) {
        $markdown_content = $markdown_content + "## Variables\n\n";
        $markdown_content = $markdown_content + "This module provides " + sizeof($module_variables) + " variable(s):\n\n";
        
        for (string $var_key, object $var_info : $module_variables) {
            string $var_name = $var_info["name"];
            $markdown_content = $markdown_content + "### " + $var_name + "\n\n";
            $markdown_content = $markdown_content + "**Variable:** `" + $var_name + "`\n\n";
        }
    }
    
    // Footer
    $markdown_content = $markdown_content + "## Notes\n\n";
    $markdown_content = $markdown_content + "- This documentation was generated automatically from the VoidScript module introspection system.\n";
    $markdown_content = $markdown_content + "- For the most up-to-date information, use the `function_doc()` and `module_info()` functions.\n";
    $markdown_content = $markdown_content + "- Module path: `" + $module_path + "`\n\n";
    
    if ($module_type == "Dynamic") {
        $markdown_content = $markdown_content + "**Dynamic Module Notes:**\n";
        $markdown_content = $markdown_content + "- This module is loaded dynamically at runtime\n";
        $markdown_content = $markdown_content + "- Ensure required dependencies are installed\n";
        $markdown_content = $markdown_content + "- Module availability depends on compilation flags and system libraries\n\n";
    }
    
    // Write the markdown file
    string $filename = $target_dir + "/" + $module_name + ".md";
    file_put_contents($filename, $markdown_content, true);
    
    printnl("Generated: ", $filename);
}

printnl("\n=== Documentation Generation Complete ===");
printnl("Documentation files generated in: ", $docs_dir);
printnl("- Built-in modules: ", $builtin_dir);
printnl("- Dynamic modules: ", $dynamic_dir);
printnl("\nTo view the documentation, check the generated .md files in each directory.");
