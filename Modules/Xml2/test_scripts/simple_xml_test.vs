printnl("=== Simple XML Test ===");

const string $xmlContent = "<?xml version=\"1.0\"?><root><item>test</item></root>";
const string $xmlFile = "/tmp/simple.xml";

file_put_contents($xmlFile, $xmlContent, false);

Xml2 $xml = new Xml2();
$xml->readFile($xmlFile);

printnl("XML file read successfully");

unlink($xmlFile);
printnl("Test complete");