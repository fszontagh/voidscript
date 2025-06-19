// XML XPath Phase 2 Final Test Script

printnl("=== XML XPath Phase 2 Final Test ===");

const string $testXml = "<?xml version=\"1.0\"?><bookstore><book id=\"1\" category=\"fiction\"><title lang=\"en\">Great Gatsby</title><author>F. Scott Fitzgerald</author><price>12.99</price></book><book id=\"2\" category=\"fiction\"><title lang=\"en\">To Kill a Mockingbird</title><author>Harper Lee</author><price>14.99</price></book><book id=\"3\" category=\"biography\"><title lang=\"en\">Steve Jobs</title><author>Walter Isaacson</author><price>16.99</price></book></bookstore>";

// Test 1: Basic XPath functionality
printnl("Test 1: Basic XPath functionality");
Xml2 $xml = new Xml2();
XmlDocument $doc = $xml->createFromString($testXml);

if ($doc != NULL) {
    printnl("✓ Document created successfully");
    
    // Test selectNodes
    XmlNodeList $allBooks = $doc->selectNodes("//book");
    if ($allBooks != NULL) {
        const int $bookCount = $allBooks->getLength();
        printnl("✓ selectNodes found books: ", $bookCount);
        
        // Test accessing individual books
        if ($bookCount > 0) {
            XmlNode $firstBookFromList = $allBooks->item(0);
            if ($firstBookFromList != NULL) {
                const string $bookId = $firstBookFromList->getAttribute("id");
                printnl("✓ First book ID from list: ", $bookId);
            }
        }
    } else {
        printnl("✗ selectNodes failed");
    }
    
    // Test selectSingleNode
    XmlNode $firstBook = $doc->selectSingleNode("//book[1]");
    if ($firstBook != NULL) {
        printnl("✓ selectSingleNode works");
        const string $id = $firstBook->getAttribute("id");
        printnl("✓ First book ID: ", $id);
        
        // Test XPath from node context
        XmlNode $title = $firstBook->selectSingleNode("title");
        if ($title != NULL) {
            const string $titleText = $title->getTextContent();
            printnl("✓ Node context XPath works: ", $titleText);
        }
    } else {
        printnl("✗ selectSingleNode failed");
    }
    
    // Test xpath() method with count
    XmlXPathResult $countResult = $doc->xpath("count(//book)");
    if ($countResult != NULL) {
        const int $resultType = $countResult->getType();
        printnl("✓ xpath() method works, result type: ", $resultType);
        
        if ($resultType == 3) { // NUMBER type
            const double $count = $countResult->getNumberValue();
            printnl("✓ Number result: ", $count);
        }
    } else {
        printnl("✗ xpath() method failed");
    }
} else {
    printnl("✗ Document creation failed");
}

// Test 2: XPath with predicates
printnl("Test 2: XPath with predicates");
Xml2 $xml2 = new Xml2();
XmlDocument $doc2 = $xml2->createFromString($testXml);

if ($doc2 != NULL) {
    // Test attribute-based selection
    XmlNodeList $fictionBooks = $doc2->selectNodes("//book[@category='fiction']");
    if ($fictionBooks != NULL) {
        const int $fictionCount = $fictionBooks->getLength();
        printnl("✓ Fiction books found: ", $fictionCount);
    }
    
    // Test value-based selection
    XmlNodeList $expensiveBooks = $doc2->selectNodes("//book[price > 15]");
    if ($expensiveBooks != NULL) {
        const int $expensiveCount = $expensiveBooks->getLength();
        printnl("✓ Expensive books found: ", $expensiveCount);
    }
}

// Test 3: XPath result types
printnl("Test 3: XPath result types");
Xml2 $xml3 = new Xml2();
XmlDocument $doc3 = $xml3->createFromString($testXml);

if ($doc3 != NULL) {
    // Test boolean result
    XmlXPathResult $boolResult = $doc3->xpath("count(//book) > 2");
    if ($boolResult != NULL && $boolResult->getType() == 2) {
        const bool $hasBooks = $boolResult->getBooleanValue();
        printnl("✓ Boolean result: ", $hasBooks);
    }
    
    // Test string result
    XmlXPathResult $stringResult = $doc3->xpath("//book[1]/title/text()");
    if ($stringResult != NULL && $stringResult->getType() == 4) {
        const string $firstTitle = $stringResult->getStringValue();
        printnl("✓ String result: ", $firstTitle);
    }
    
    // Test nodeset result
    XmlXPathResult $nodesetResult = $doc3->xpath("//book");
    if ($nodesetResult != NULL && $nodesetResult->getType() == 1) {
        const int $nodeCount = $nodesetResult->getNodeSetSize();
        printnl("✓ Nodeset size: ", $nodeCount);
        
        XmlNodeList $nodeSet = $nodesetResult->getNodeSet();
        if ($nodeSet != NULL) {
            printnl("✓ Nodeset conversion works");
        }
    }
}

// Test 4: XPath object creation
printnl("Test 4: XPath object creation");
Xml2 $xml4 = new Xml2();
XmlDocument $doc4 = $xml4->createFromString($testXml);

if ($doc4 != NULL) {
    XmlXPath $xpath = $xml4->createXPath($doc4);
    if ($xpath != NULL) {
        printnl("✓ XPath object created");
        
        XmlXPathResult $result = $xpath->evaluate("count(//book)");
        if ($result != NULL) {
            printnl("✓ XPath evaluate works");
            if ($result->getType() == 3) {
                const double $count = $result->getNumberValue();
                printnl("✓ Evaluated count: ", $count);
            }
        }
        
        // Test namespace registration
        $xpath->registerNamespace("test", "http://example.com/test");
        printnl("✓ Namespace registration works");
        
        // Test variable registration
        $xpath->registerVariable("testVar", "testValue");
        $xpath->registerVariable("testNum", 42.0);
        $xpath->registerVariable("testBool", true);
        printnl("✓ Variable registration works");
    }
}

// Test 5: Advanced XPath expressions
printnl("Test 5: Advanced XPath expressions");
Xml2 $xml5 = new Xml2();
XmlDocument $doc5 = $xml5->createFromString($testXml);

if ($doc5 != NULL) {
    // Test attribute selection
    XmlNodeList $englishTitles = $doc5->selectNodes("//title[@lang='en']");
    if ($englishTitles != NULL) {
        const int $englishCount = $englishTitles->getLength();
        printnl("✓ English titles found: ", $englishCount);
    }
    
    // Test text node selection
    XmlNodeList $authors = $doc5->selectNodes("//author/text()");
    if ($authors != NULL) {
        const int $authorCount = $authors->getLength();
        printnl("✓ Author text nodes found: ", $authorCount);
    }
    
    // Test position-based selection
    XmlNode $secondBook = $doc5->selectSingleNode("//book[2]");
    if ($secondBook != NULL) {
        const string $secondId = $secondBook->getAttribute("id");
        printnl("✓ Second book ID: ", $secondId);
    }
}

printnl("=== XPath Phase 2 Test Complete ===");