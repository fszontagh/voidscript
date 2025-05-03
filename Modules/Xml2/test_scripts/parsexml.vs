const string $xml_url = "https://www.w3schools.com/xml/simple.xml";
const string $xml_filename = "/tmp/simple_test.xml";

if (file_exists($xml_filename) == false) {
    const string $xml_content = curlGet($xml_url);
    file_put_contents($xml_filename, $xml_content, true);
}else{
    printnl("Test file exists: ", $xml_filename);
}

MariaDB $db = new MariaDB();
$db->connect("127.0.0.1", "fszontagh", "ibjazq87", "employees");

if ($db != NULL) {
    printnl("DB connection established");
}
object[] $results = $db->query("SELECT * FROM employees LIMIT 2");

if ($results != NULL) {
    for (object $result : $results) {
        printnl("Name: ", $result->first_name);
    }
}

$db->close();
Xml2 $xml = new Xml2();

printnl("Testing file: ", $xml_filename);

$xml->readFile($xml_filename);
XmlNode $node = $xml->getRootElement();
object $properties = $node->getAttributes();


if ($properties->content == NULL) {
    format("Root node name: {} type: {} content is NULL \n", $properties->name, $properties->type );
}else{
    format("Root node name: {} type: {} content: {}\n", $properties->name, $properties->type, $properties->content);
}

while ($node->hasNext()) {

}

//for (string $key, auto $val : $properties) {
    //    printnl("Key: ", $key, " val: ",$val);
    //
    //}