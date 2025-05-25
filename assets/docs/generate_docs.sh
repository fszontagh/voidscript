#!/bin/bash

# VoidScript Module Documentation Generator
# Usage: ./generate_docs.sh <output_directory>
# 
# This script generates comprehensive markdown documentation for all VoidScript modules
# including both built-in and dynamic modules with their classes, methods, and functions.

set -e

# Check if argument is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <output_directory>"
    echo "Example: $0 /path/to/docs"
    exit 1
fi

OUTPUT_DIR="$1"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

echo "VoidScript Documentation Generator"
echo "================================="
echo "Project Root: $PROJECT_ROOT"
echo "Build Directory: $BUILD_DIR"
echo "Output Directory: $OUTPUT_DIR"
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory not found at $BUILD_DIR"
    echo "Please build the project first using: cmake --build build"
    exit 1
fi

# Check if voidscript binary exists
if [ ! -f "$BUILD_DIR/voidscript" ]; then
    echo "Error: voidscript binary not found at $BUILD_DIR/voidscript"
    echo "Please build the project first using: cmake --build build"
    exit 1
fi

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Create subdirectories for built-in and dynamic modules
BUILTIN_DIR="$OUTPUT_DIR/builtin-modules"
DYNAMIC_DIR="$OUTPUT_DIR/dynamic-modules"
mkdir -p "$BUILTIN_DIR"
mkdir -p "$DYNAMIC_DIR"

echo "Created documentation directories:"
echo "  - Built-in modules: $BUILTIN_DIR"
echo "  - Dynamic modules: $DYNAMIC_DIR"
echo ""

# Create VoidScript documentation extractor
DOC_EXTRACTOR="$OUTPUT_DIR/doc_extractor.vs"
cat > "$DOC_EXTRACTOR" << 'EOF'
// VoidScript Documentation Extractor
// This script extracts comprehensive module documentation

printnl("=== VoidScript Module Documentation Extractor ===");
printnl("");

// Get all available modules
object $modules = module_list();

// Function to safely get module info
function safe_module_info($module_name) {
    try {
        return module_info($module_name);
    } catch {
        return {};
    }
}

// Function to safely get function documentation
function safe_function_doc($func_name) {
    try {
        return function_doc($func_name);
    } catch {
        return {};
    }
}

// Process each module
object $all_module_data = {};
foreach ($modules as $module_name => $module_data) {
    printnl("Processing module: ", $module_name);
    
    object $detailed_info = safe_module_info($module_name);
    $all_module_data[$module_name] = $detailed_info;
    
    // Add function details
    if (typeof($module_data["functions"], "object")) {
        object $function_details = {};
        foreach ($module_data["functions"] as $func_name => $func_info) {
            object $func_doc = safe_function_doc($func_name);
            $function_details[$func_name] = $func_doc;
        }
        $all_module_data[$module_name]["function_details"] = $function_details;
    }
}

// Output the complete module data as JSON for processing
printnl("=== MODULE_DATA_START ===");
printnl(json_encode($all_module_data));
printnl("=== MODULE_DATA_END ===");

printnl("");
printnl("Documentation extraction complete.");
EOF

echo "Created documentation extractor script: $DOC_EXTRACTOR"

# Run the documentation extractor and capture output
echo "Extracting module information..."
cd "$BUILD_DIR"
DOC_OUTPUT=$(./voidscript "$DOC_EXTRACTOR" 2>/dev/null)

# Extract JSON data between markers
MODULE_JSON=$(echo "$DOC_OUTPUT" | sed -n '/=== MODULE_DATA_START ===/,/=== MODULE_DATA_END ===/p' | grep -v "=== MODULE_DATA")

if [ -z "$MODULE_JSON" ]; then
    echo "Error: Failed to extract module data"
    echo "Output was:"
    echo "$DOC_OUTPUT"
    exit 1
fi

echo "Successfully extracted module data"

# Create a Python script to process the JSON and generate markdown
PROCESSOR="$OUTPUT_DIR/process_docs.py"
cat > "$PROCESSOR" << 'EOF'
#!/usr/bin/env python3

import json
import sys
import os
from pathlib import Path

def escape_markdown(text):
    """Escape special markdown characters in text"""
    if not isinstance(text, str):
        return str(text)
    # Escape common markdown characters
    chars_to_escape = ['\\', '`', '*', '_', '{', '}', '[', ']', '(', ')', '#', '+', '-', '.', '!']
    for char in chars_to_escape:
        text = text.replace(char, '\\' + char)
    return text

def generate_module_doc(module_name, module_data, is_dynamic=False):
    """Generate markdown documentation for a module"""
    
    # Determine module type
    module_type = "Dynamic Module" if is_dynamic else "Built-in Module"
    
    doc = f"""# {module_name}

**Type:** {module_type}  
**Path:** {module_data.get('path', 'N/A')}

## Overview

{module_data.get('description', f'Documentation for the {module_name} module.')}

"""

    # Add classes section if classes exist
    if 'classes' in module_data and module_data['classes']:
        doc += "## Classes\n\n"
        for class_name in module_data['classes']:
            doc += f"### {class_name}\n\n"
            doc += f"The `{class_name}` class provides object-oriented functionality for this module.\n\n"
            
            # Add class methods if available
            class_methods = module_data.get('class_methods', {}).get(class_name, [])
            if class_methods:
                doc += f"#### Methods\n\n"
                for method in class_methods:
                    doc += f"- `{method}`\n"
                doc += "\n"

    # Add functions section
    if 'functions' in module_data and module_data['functions']:
        doc += "## Functions\n\n"
        
        function_details = module_data.get('function_details', {})
        
        for func_name, func_info in module_data['functions'].items():
            doc += f"### {func_name}\n\n"
            
            # Get detailed function documentation
            func_detail = function_details.get(func_name, {})
            
            # Add description
            description = func_detail.get('description', func_info.get('description', 'No description available.'))
            doc += f"**Description:** {description}\n\n"
            
            # Add parameters
            parameters = func_info.get('parameters', [])
            if parameters:
                doc += "**Parameters:**\n\n"
                for param in parameters:
                    param_name = param.get('name', 'unknown')
                    param_type = param.get('type', 'unknown')
                    param_desc = param.get('description', 'No description')
                    doc += f"- `{param_name}` (*{param_type}*): {param_desc}\n"
                doc += "\n"
            else:
                doc += "**Parameters:** None\n\n"
            
            # Add return type
            return_type = func_detail.get('return_type', func_info.get('return_type', 'unknown'))
            doc += f"**Returns:** {return_type}\n\n"
            
            # Add usage example if available
            example = func_detail.get('example')
            if example:
                doc += f"**Example:**\n\n```voidscript\n{example}\n```\n\n"
            
            doc += "---\n\n"

    return doc

def categorize_modules(modules_data):
    """Categorize modules into built-in and dynamic"""
    builtin_modules = {}
    dynamic_modules = {}
    
    for module_name, module_data in modules_data.items():
        # Determine if module is dynamic based on available classes or specific functions
        has_dynamic_classes = 'classes' in module_data and len(module_data.get('classes', [])) > 0
        has_dynamic_functions = any(func in module_data.get('functions', {}) for func in 
                                  ['curlGet', 'curlPost', 'format', 'format_print'])
        
        if has_dynamic_classes or has_dynamic_functions:
            # Check if it has typical dynamic module classes
            dynamic_class_names = {'MariaDB', 'Imagick', 'Xml2', 'XmlNode', 'XmlAttr'}
            module_classes = set(module_data.get('classes', []))
            
            if module_classes.intersection(dynamic_class_names) or has_dynamic_functions:
                dynamic_modules[module_name] = module_data
            else:
                builtin_modules[module_name] = module_data
        else:
            builtin_modules[module_name] = module_data
    
    return builtin_modules, dynamic_modules

def main():
    if len(sys.argv) != 4:
        print("Usage: python3 process_docs.py <json_data> <builtin_dir> <dynamic_dir>")
        sys.exit(1)
    
    json_data = sys.argv[1]
    builtin_dir = sys.argv[2]
    dynamic_dir = sys.argv[3]
    
    try:
        modules_data = json.loads(json_data)
    except json.JSONDecodeError as e:
        print(f"Error parsing JSON: {e}")
        sys.exit(1)
    
    # Categorize modules
    builtin_modules, dynamic_modules = categorize_modules(modules_data)
    
    print(f"Found {len(builtin_modules)} built-in modules and {len(dynamic_modules)} dynamic modules")
    
    # Generate documentation for built-in modules
    for module_name, module_data in builtin_modules.items():
        doc = generate_module_doc(module_name, module_data, is_dynamic=False)
        filename = f"{module_name.lower().replace(' ', '_')}.md"
        filepath = os.path.join(builtin_dir, filename)
        
        with open(filepath, 'w') as f:
            f.write(doc)
        print(f"Generated: {filepath}")
    
    # Generate documentation for dynamic modules
    for module_name, module_data in dynamic_modules.items():
        doc = generate_module_doc(module_name, module_data, is_dynamic=True)
        filename = f"{module_name.lower().replace(' ', '_')}.md"
        filepath = os.path.join(dynamic_dir, filename)
        
        with open(filepath, 'w') as f:
            f.write(doc)
        print(f"Generated: {filepath}")
    
    # Generate index files
    generate_index_file(builtin_dir, builtin_modules, "Built-in Modules")
    generate_index_file(dynamic_dir, dynamic_modules, "Dynamic Modules")
    
    # Generate main index
    generate_main_index(os.path.dirname(builtin_dir), builtin_modules, dynamic_modules)

def generate_index_file(directory, modules, title):
    """Generate an index file for a module category"""
    index_path = os.path.join(directory, "README.md")
    
    doc = f"""# {title}

This directory contains documentation for all {title.lower()}.

## Available Modules

"""
    
    for module_name, module_data in modules.items():
        filename = f"{module_name.lower().replace(' ', '_')}.md"
        description = module_data.get('description', f'Documentation for {module_name}')
        doc += f"- [{module_name}]({filename}) - {description}\n"
    
    doc += f"""

## Summary

- **Total modules:** {len(modules)}
- **Documentation generated:** {len(modules)} files

"""
    
    with open(index_path, 'w') as f:
        f.write(doc)
    print(f"Generated index: {index_path}")

def generate_main_index(output_dir, builtin_modules, dynamic_modules):
    """Generate main documentation index"""
    index_path = os.path.join(output_dir, "README.md")
    
    doc = f"""# VoidScript Module Documentation

This documentation covers all available VoidScript modules, including both built-in and dynamic modules.

## Documentation Structure

### Built-in Modules
Built-in modules are core modules that are always available in VoidScript.

- **Location:** [`builtin-modules/`](builtin-modules/)
- **Count:** {len(builtin_modules)} modules

### Dynamic Modules  
Dynamic modules are loaded at runtime and provide extended functionality.

- **Location:** [`dynamic-modules/`](dynamic-modules/)
- **Count:** {len(dynamic_modules)} modules

## Quick Links

### Built-in Modules
"""
    
    for module_name in sorted(builtin_modules.keys()):
        filename = f"{module_name.lower().replace(' ', '_')}.md"
        doc += f"- [{module_name}](builtin-modules/{filename})\n"
    
    doc += "\n### Dynamic Modules\n"
    
    for module_name in sorted(dynamic_modules.keys()):
        filename = f"{module_name.lower().replace(' ', '_')}.md"
        doc += f"- [{module_name}](dynamic-modules/{filename})\n"
    
    doc += f"""

## Statistics

- **Total modules documented:** {len(builtin_modules) + len(dynamic_modules)}
- **Built-in modules:** {len(builtin_modules)}
- **Dynamic modules:** {len(dynamic_modules)}

## About

This documentation was automatically generated using the VoidScript documentation generator.
For more information about VoidScript, visit the main project repository.
"""
    
    with open(index_path, 'w') as f:
        f.write(doc)
    print(f"Generated main index: {index_path}")

if __name__ == "__main__":
    main()
EOF

chmod +x "$PROCESSOR"

echo "Processing module data and generating documentation..."

# Run the Python processor
python3 "$PROCESSOR" "$MODULE_JSON" "$BUILTIN_DIR" "$DYNAMIC_DIR"

# Clean up temporary files
rm -f "$DOC_EXTRACTOR" "$PROCESSOR"

echo ""
echo "Documentation generation complete!"
echo "========================================"
echo "Output directory: $OUTPUT_DIR"
echo "Built-in modules: $BUILTIN_DIR"
echo "Dynamic modules: $DYNAMIC_DIR"
echo ""
echo "You can now browse the generated documentation:"
echo "  - Main index: $OUTPUT_DIR/README.md"
echo "  - Built-in modules: $BUILTIN_DIR/README.md"
echo "  - Dynamic modules: $DYNAMIC_DIR/README.md"
echo ""
