#!/usr/bin/env voidscript
printnl("=== VoidScript Documentation Generator ===");

string $docs_dir = "generated_docs";

if (!isset($argv[1])) {
    error("Usage: ", $argv[0], " target_dir");
    exit(1);
}

$docs_dir = $argv[1];

printnl("Generating documentation in directory: ", $docs_dir);

// Create main documentation directory
if (!file_exists($docs_dir)) {
    printnl("Creating documentation directory: ", $docs_dir);
    mkdir($docs_dir, true);
}

// Create subdirectories for builtin and dynamic modules
string $builtin_dir = $docs_dir + "/builtin_modules";
string $dynamic_dir = $docs_dir + "/dynamic_modules";

if (!file_exists($builtin_dir)) {
    mkdir($builtin_dir, true);
}

if (!file_exists($dynamic_dir)) {
    mkdir($dynamic_dir, true);
    printnl("Documentation directories created.");
}

// Function to generate markdown content for a module
function generateModuleMarkdown(string $module_name, object $info) string {
    string $content = "# " + $module_name + "\n\n";
    
    // Add classes section
    if (!is_null($info["classes"]) && typeof($info["classes"]) == "object") {
        $content = $content + "## Classes\n\n";
        // Note: This may still have iteration issues, but we'll handle what we can
        for (string $class_name, object $class : $info["classes"]) {
            $content = $content + "### " + $class_name + "\n\n";
            
            // Add methods if available
            if (!is_null($class["methods"]) && typeof($class["methods"]) == "object") {
                $content = $content + "**Methods:**\n";
                for (string $method_name, object $method : $class["methods"]) {
                    string $params = "";
                    if (!is_null($method["parameters"])) {
                        $params = $method["parameters"];
                    }
                    string $return_type = "";
                    if (!is_null($method["return_type"])) {
                        $return_type = ": " + $method["return_type"];
                    }
                    $content = $content + "- `" + $method_name + "(" + $params + ")" + $return_type + "`\n";
                }
                $content = $content + "\n";
            }
        }
    }
    
    // Add functions section
    if (!is_null($info["functions"]) && typeof($info["functions"]) == "object") {
        $content = $content + "## Functions\n\n";
        for (string $function_name, object $function : $info["functions"]) {
            string $params = "";
            if (!is_null($function["parameters"])) {
                $params = $function["parameters"];
            }
            string $return_type = "";
            if (!is_null($function["return_type"])) {
                $return_type = ": " + $function["return_type"];
            }
            $content = $content + "- `" + $function_name + "(" + $params + ")" + $return_type + "`\n";
        }
        $content = $content + "\n";
    }
    
    // Add constants section
    if (!is_null($info["constants"]) && typeof($info["constants"]) == "object") {
        $content = $content + "## Constants\n\n";
        for (string $const_name, string $const_value : $info["constants"]) {
            $content = $content + "- `" + $const_name + ": " + $const_value + "`\n";
        }
        $content = $content + "\n";
    }
    
    return $content;
}

// Function to determine if a module is built-in or dynamic
function isBuiltinModule(string $module_name) bool {
    // List of known built-in modules (without "Module" suffix)
    string[] $builtin_modules = ["Array", "String", "File", "Json", "Print", "Conversion", "VariableHelpers", "ModuleHelper"];
    
    for (int $i = 0; $i < sizeof($builtin_modules); $i++) {
        if ($module_name == $builtin_modules[$i]) {
            return true;
        }
    }
    return false;
}

// Function to write documentation file
function writeDocumentationFile(string $file_path, string $content) {
    file_put_contents($file_path, $content, true);
    printnl("  Generated: ", $file_path);
}

printnl("Analyzing modules...");

// Hardcoded list of known module names to work around module_list() iteration bug
string[] $builtin_modules = ["Array", "String", "File", "Json", "Print", "Conversion", "VariableHelpers", "ModuleHelper"];
string[] $dynamic_modules = ["Curl", "Format", "Imagick", "MariaDb", "Xml2"];

// Combine all known modules (VoidScript doesn't support array append syntax)
string[] $all_modules = ["Array", "String", "File", "Json", "Print", "Conversion", "VariableHelpers", "ModuleHelper", "Curl", "Format", "Imagick", "MariaDb", "Xml2"];

// Generate documentation for each known module (using index-based loop to avoid for-each bug)
string $module_name = "";
for (int $i = 0; $i < sizeof($all_modules); $i++) {
    $module_name = $all_modules[$i];
    printnl("Processing module: ", $module_name);
    
    // Get module info with basic error handling
    object $info = module_info($module_name);
    
    if (is_null($info)) {
        printnl("  Warning: No information available for module: ", $module_name);
        continue;
    }
    
    // Generate markdown content
    string $markdown_content = generateModuleMarkdown($module_name, $info);
    
    // Determine target directory based on module type
    string $target_dir = "";
    if (isBuiltinModule($module_name)) {
        $target_dir = $builtin_dir;
    } else {
        $target_dir = $dynamic_dir;
    }
    string $file_path = $target_dir + "/" + $module_name + ".md";
    
    // Write the documentation file
    writeDocumentationFile($file_path, $markdown_content);
}

printnl("\n=== Documentation Generation Complete ===");
printnl("Documentation files generated in: ", $docs_dir);
printnl("- Built-in modules: ", $builtin_dir);
printnl("- Dynamic modules: ", $dynamic_dir);
printnl("\nTo view the documentation, check the generated .md files in each directory.");
