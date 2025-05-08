const string $xml_file_name = string_replace($argv[0],"xml.vs","books.xml", false);

if (!file_exists($xml_file_name)) {
    throw_error("Xml file not found: ", $xml_file_name);
}

printnl("file_name: ", $xml_file_name);
const string $xml_test_string = file_get_contents($xml_file_name);

printnl("Xml length: ", string_length($xml_test_string), " File size: ", file_size($xml_file_name));

if (!module_exists("modules-xml2")) {
    throw_error("Xml2 module not found");
}

module_print_info("modules-xml2");
Xml2 $xml = new Xml2();

$xml->readMemory($xml_test_string);
XmlNode $node = $xml->getRootElement();

const object $attrs = $node->getAttributes();

printnl("------~ ",$attrs->tagName," ~-------");
object $child_attr = {};
for (object $child : $attrs->children) {
    $child_attr = $child->getAttributes();

    for (string $child_key, auto $child_value : $child_attr) {
        if (typeof($child_value,"object") == false) {
            if ($child_value == NULL) {
                printnl("child_Key: ", $child_key, " Value: --");
            } else {
                printnl("child_Key: ", $child_key, " Value: ", $child_value);
            }
        } else {
            printnl("child_Key: ", $child_key, " is an object");
        }
    }
    printnl("-----");
}


