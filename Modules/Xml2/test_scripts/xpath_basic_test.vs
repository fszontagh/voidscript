// Basic XPath Test Script

printnl("=== XPath Basic Test ===");

const string $xml = "<?xml version=\"1.0\"?><books><book id=\"1\"><title>Test Book</title><author>Test Author</author></book><book id=\"2\"><title>Another Book</title><author>Another Author</author></book></books>";

Xml2 $xmlHandler = new Xml2();
XmlDocument $doc = $xmlHandler->createFromString($xml);

if ($doc != NULL) {
    printnl("Document created successfully");
    
    // Test selectNodes
    XmlNodeList $books = $doc->selectNodes("//book");
    if ($books != NULL) {
        int $count = $books->getLength();
        printnl("Found books: ", $count);
    }
    
    // Test selectSingleNode  
    XmlNode $firstBook = $doc->selectSingleNode("//book[1]");
    if ($firstBook != NULL) {
        string $id = $firstBook->getAttribute("id");
        printnl("First book ID: ", $id);
        
        XmlNode $title = $firstBook->selectSingleNode("title");
        if ($title != NULL) {
            string $titleText = $title->getTextContent();
            printnl("Title: ", $titleText);
        }
    }
    
    // Test xpath method
    XmlXPathResult $result = $doc->xpath("count(//book)");
    if ($result != NULL) {
        int $type = $result->getType();
        printnl("XPath result type: ", $type);
        
        if ($type == 3) {
            double $bookCount = $result->getNumberValue();
            printnl("Book count: ", $bookCount);
        }
    }
    
    printnl("✓ XPath functionality works!");
} else {
    printnl("✗ Document creation failed");
}

printnl("=== Test Complete ===");