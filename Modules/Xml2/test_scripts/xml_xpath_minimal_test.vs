// Minimal XPath Test

printnl("=== XML XPath Minimal Test ===");

const string $testXml : "<?xml version=\"1.0\"?><books><book id=\"1\"><title>Test Book</title></book></books>";

try {
    Xml2 $xml : new Xml2();
    XmlDocument $doc : $xml->createFromString($testXml);
    
    if ($doc != NULL) {
        printnl("✓ Document created");
        
        XmlNodeList $books : $doc->selectNodes("//book");
        if ($books != NULL) {
            const int $count : $books->getLength();
            printnl("✓ selectNodes found books: ", $count);
        }
        
        XmlNode $book : $doc->selectSingleNode("//book[1]");
        if ($book != NULL) {
            printnl("✓ selectSingleNode works");
        }
        
        XmlXPathResult $result : $doc->xpath("count(//book)");
        if ($result != NULL) {
            const int $type : $result->getType();
            printnl("✓ xpath() works, type: ", $type);
        }
    }
} catch (string $error) {
    printnl("✗ Error: ", $error);
}

printnl("=== Test Complete ===");