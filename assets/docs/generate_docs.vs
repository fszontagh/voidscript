
if (!isset($argv[1])) {
    error("Usage: ", $argv[0], " target_directory");
    exit(1);
}

string $target_directory = $argv[1];

if (!file_exists($target_directory)) {
    mkdir($target_directory);
}

function formatFunctions(string[] $functions) string {
    string $doc_output = "##### Functions  \n";
    for (string $function : $functions) {
        $doc_output+= "######  " + $function + " \n";
        object $finfo  = get_function_details($function);
        //print(var_dump($finfo));
        $doc_output+= $finfo["description"] + "\n";

        $doc_output+= format("```void\n{}(", $function);

        if (sizeof($finfo["parameters"]) > 0) {
            for (object $param : $finfo["parameters"]) {
                $doc_output+= $param["type"]+" ";
                $doc_output+="\$"+$param["name"];
                if ($param["interpolate"] == true) {
                    $doc_output+=" ...";
                }
                $doc_output+=",";
            }
            $doc_output = string_substr($doc_output,0, string_length($doc_output)-1);
        }

        $doc_output+=")";
        $doc_output+= format(" -> {} \n```\n",$finfo["return_type"]);

        if (sizeof($finfo["parameters"])>0) {
            $doc_output+="\n**Parameters:**  \n";

            for (object $param : $finfo["parameters"]) {
                $doc_output+="- `\$"+$param["name"]+"`";
                if ($param["optional"] == true)  {
                    $doc_output+= " *optional* ";
                }
                $doc_output+= " `" + $param["type"] + "` \n";
                $doc_output+=" *" + $param["description"] + "*  \n";
            }
        }

        $doc_output+= format("\n**Return type:** `{}`\n", $finfo["return_type"]);
    }
    return $doc_output;
}

function formatClasses(string[] $classes) string {
    string $doc_output = "### Classes  \n";
    for (string $class : $classes) {
        $doc_output+="#### "+$class+"  \n";
        //print(var_dump(get_class_details($class)));
        object $class_details = get_class_details($class);
        $doc_output+= "##### Methods  \n";

        for (string $method : $class_details["methods"]) {
            object $method_detail = get_method_details($class, $method);

            $doc_output+= "######  " + $method_detail["qualified_name"] + "  \n";
            $doc_output+="Visibility: ";
            if ($method_detail["is_private"] == false) {
                $doc_output+= "public";
            }else{
                $doc_output+= "private";
            }
            $doc_output+="  \n\n";
            $doc_output+= $method_detail["description"]+"  \n";
            $doc_output+= "```void\n";

            $doc_output+= format("{}->{}(", $method_detail["class"], $method_detail["name"]);
            if (sizeof($method_detail["parameters"])>0) {
                for (object $param : $method_detail["parameters"]){
                    $doc_output+=format("{} ",$param["type"]);
                    $doc_output+="\$"+$param["name"];
                    if ($param["interpolate"] == true) {
                        $doc_output+=" ...";
                    }
                    $doc_output+=",";
                }
                $doc_output = string_substr($doc_output, 0, string_length($doc_output) - 1);
            }
            $doc_output+= ")";
            //print(var_dump($method_detail));
            $doc_output+=" -> " + $method_detail["return_type"]+" ";
            $doc_output+="\n```\n";

            if (sizeof($method_detail["parameters"])>0) {
                $doc_output+="\n**Parameters:**  \n";

                for (object $param : $method_detail["parameters"]) {
                    $doc_output+="- `\$"+$param["name"]+"`";
                    if ($param["optional"] == true)  {
                        $doc_output+= " *optional* ";
                    }
                    $doc_output+= " `" + $param["type"] + "` \n";
                    $doc_output+=" *" + $param["description"] + "*  \n";
                }
            }
            $doc_output+= format("\n**Return type:** `{}`\n", $method_detail["return_type"]);
        }

    }

    return $doc_output;
}

string[] $modules = list_modules();

for (string $module : $modules) {
    string $md_filename = $target_directory + "/" + $module + ".md";
    string $doc_output = format("# {} module\n", $module);

    // Add module description if available
    string $module_description = get_module_description($module);
    if (string_length($module_description) > 0) {
        $doc_output += "\n" + $module_description + "\n\n";
    }

    string[] $functions = list_module_functions($module);

    if (sizeof($functions)>0) {
        $doc_output+=formatFunctions($functions);
    }

    string[] $classes = list_module_classes($module);
    if (sizeof($classes)>0) {
        $doc_output+=formatClasses($classes);
    }

    $doc_output += "\n----\n *Generated at: " + date()+"*\n";
    file_put_contents($md_filename, $doc_output, true);
    printnl("Module docs written into: ", $md_filename);

}

