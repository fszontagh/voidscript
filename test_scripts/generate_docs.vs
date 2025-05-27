#!/usr/bin/env voidscript
// VoidScript Documentation Generator
// This script generates comprehensive markdown documentation for all loaded modules

// Check if output directory argument is provided
if (argc < 2) {
    printnl("Usage: voidscript generate_docs.vs <output_directory>");
    printnl("Example: voidscript generate_docs.vs ../docs");
    exit(1);
}

let outputDir = argv[1];
printnl("Generating documentation in directory: " + outputDir);

// Create the docs directory if it doesn't exist
mkdir(outputDir);

// Get list of all loaded modules
let modules = module_list();
printnl("Retrieved module list, analyzing structure...");

// Create main index file
let indexContent = "# VoidScript Modules Documentation\n\n";
indexContent = indexContent + "This documentation covers all available modules in VoidScript.\n\n";
indexContent = indexContent + "## Available Modules\n\n";

// Generate documentation for each module
// Since modules are stored with numeric string keys, we need to iterate through them
let moduleCount = 0;
while (true) {
    let moduleKey = string(moduleCount);
    if (!modules[moduleKey]) {
        break; // No more modules
    }
    
    let module = modules[moduleKey];
    let moduleName = module["name"];
    let modulePath = module["path"];
    
    printnl("Generating documentation for module: " + moduleName);
    
    // Add to index
    indexContent = indexContent + "- [" + moduleName + "](./" + moduleName + ".md)\n";
    
    // Create module documentation file
    let docContent = "# " + moduleName + " Module Documentation\n\n";
    docContent = docContent + "**Path:** " + modulePath + "\n\n";
    
    // Document functions
    let functions = module["functions"];
    if (functions) {
        let functionCount = 0;
        let hasFunctions = false;
        
        // Count functions first
        while (true) {
            let funcKey = string(functionCount);
            if (!functions[funcKey]) {
                break;
            }
            if (!hasFunctions) {
                docContent = docContent + "## Functions\n\n";
                hasFunctions = true;
            }
            
            let func = functions[funcKey];
            let funcName = func["name"];
            let funcDoc = func["documentation"];
            
            docContent = docContent + "### " + funcName + "\n\n";
            
            if (funcDoc) {
                if (funcDoc["description"]) {
                    docContent = docContent + funcDoc["description"] + "\n\n";
                }
                
                if (funcDoc["return_type"]) {
                    docContent = docContent + "**Returns:** " + funcDoc["return_type"] + "\n\n";
                }
                
                // Document parameters
                let params = funcDoc["parameters"];
                if (params) {
                    let paramNames = object_keys(params);
                    if (array_size(paramNames) > 0) {
                        docContent = docContent + "**Parameters:**\n\n";
                        for (let i = 0; i < array_size(paramNames); i = i + 1) {
                            let paramName = paramNames[i];
                            let param = params[paramName];
                            docContent = docContent + "- `" + param["name"] + "` (" + param["type"] + ")";
                            if (param["optional"]) {
                                docContent = docContent + " [optional]";
                            }
                            if (param["description"]) {
                                docContent = docContent + ": " + param["description"];
                            }
                            docContent = docContent + "\n";
                        }
                        docContent = docContent + "\n";
                    }
                }
            } else {
                docContent = docContent + "Function documentation not available.\n\n";
            }
            
            docContent = docContent + "---\n\n";
            functionCount = functionCount + 1;
        }
    }
    
    // Document classes
    let classes = module["classes"];
    if (classes) {
        let classCount = 0;
        let hasClasses = false;
        
        while (true) {
            let classKey = string(classCount);
            if (!classes[classKey]) {
                break;
            }
            if (!hasClasses) {
                docContent = docContent + "## Classes\n\n";
                hasClasses = true;
            }
            
            let cls = classes[classKey];
            let className = cls["name"];
            
            docContent = docContent + "### " + className + " Class\n\n";
            
            // Document methods
            let methods = cls["methods"];
            if (methods) {
                let methodCount = 0;
                let hasMethods = false;
                
                while (true) {
                    let methodKey = string(methodCount);
                    if (!methods[methodKey]) {
                        break;
                    }
                    if (!hasMethods) {
                        docContent = docContent + "#### Methods\n\n";
                        hasMethods = true;
                    }
                    
                    let method = methods[methodKey];
                    let methodName = method["name"];
                    let methodDoc = method["documentation"];
                    
                    docContent = docContent + "##### " + methodName + "\n\n";
                    
                    if (methodDoc) {
                        if (methodDoc["description"]) {
                            docContent = docContent + methodDoc["description"] + "\n\n";
                        }
                        
                        if (methodDoc["return_type"]) {
                            docContent = docContent + "**Returns:** " + methodDoc["return_type"] + "\n\n";
                        }
                        
                        // Document parameters
                        let params = methodDoc["parameters"];
                        if (params) {
                            let paramNames = object_keys(params);
                            if (array_size(paramNames) > 0) {
                                docContent = docContent + "**Parameters:**\n\n";
                                for (let i = 0; i < array_size(paramNames); i = i + 1) {
                                    let paramName = paramNames[i];
                                    let param = params[paramName];
                                    docContent = docContent + "- `" + param["name"] + "` (" + param["type"] + ")";
                                    if (param["optional"]) {
                                        docContent = docContent + " [optional]";
                                    }
                                    if (param["description"]) {
                                        docContent = docContent + ": " + param["description"];
                                    }
                                    docContent = docContent + "\n";
                                }
                                docContent = docContent + "\n";
                            }
                        }
                    } else {
                        docContent = docContent + "Method documentation not available.\n\n";
                    }
                    
                    docContent = docContent + "---\n\n";
                    methodCount = methodCount + 1;
                }
            }
            classCount = classCount + 1;
        }
    }
    
    // Write documentation file
    let filename = outputDir + "/" + moduleName + ".md";
    write_file(filename, docContent);
    printnl("Generated documentation for module: " + moduleName + " -> " + filename);
    
    moduleCount = moduleCount + 1;
}

// Write index file
let indexFilename = outputDir + "/README.md";
write_file(indexFilename, indexContent);
printnl("Generated index file: " + indexFilename);

printnl("");
printnl("Documentation generation completed!");
printnl("Total modules documented: " + string(moduleCount));
printnl("Output directory: " + outputDir);