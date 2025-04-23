const string $xml_url = "https://www.w3schools.com/xml/note.xml";
const string $xml_filename = "/tmp/test.xml";

if (file_exists($xml_filename) == false) {
    const string $xml_content = curlGet($xml_url);
    file_put_contents($xml_filename, $xml_content, true);
}else{
    printnl("Test file exists: ", $xml_filename);
}

Xml2 $xml = new Xml2();
$xml->constructor();

printnl("Testing file: ", $xml_filename);

$xml->readFile($xml_filename);