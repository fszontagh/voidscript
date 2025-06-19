// XML XPath Phase 2 Test Script

printnl("=== XML XPath Phase 2 Test ===");

// Test XML content
const string $testXmlContent = "<?xml version=\"1.0\"?><bookstore><book id=\"1\" category=\"fiction\"><title lang=\"en\">Great Gatsby</title><author>F. Scott Fitzgerald</author><price>12.99</price></book><book id=\"2\" category=\"fiction\"><title lang=\"en\">To Kill a Mockingbird</title><author>Harper Lee</author><price>14.99</price></book><book id=\"3\" category=\"biography\"><title lang=\"en\">Steve Jobs</title><author>Walter Isaacson</author><price>16.99</price></book></bookstore>";

// Test 1: Create document and test basic XPath
printnl("Test 1: Basic XPath functionality");
try {
    Xml2 $xml : new Xml2();
    XmlDocument $doc : $xml->createFromString($testXmlContent);
    
    if ($doc != NULL) {
        printnl("✓ Document created successfully");
        
        // Test selectNodes
        XmlNodeList $allBooks : $doc->selectNodes("//book");
        if ($allBooks != NULL) {
            const int $bookCount : $allBooks->getLength();
            printnl("✓ selectNodes found books: ", $bookCount);
        } else {
            printnl("✗ selectNodes failed");
        }
        
        // Test selectSingleNode
        XmlNode $firstBook = $doc->selectSingleNode("//book[1]");
        if ($firstBook != NULL) {
            printnl("✓ selectSingleNode works");
            
            // Test XPath from node context
            XmlNode $title = $firstBook->selectSingleNode("title");
            if ($title != NULL) {
                const string $titleText = $title->getTextContent();
                printnl("✓ Node context XPath works: ", $titleText);
            }
        } else {
            printnl("✗ selectSingleNode failed");
        }
        
        // Test xpath() method
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
} catch (string $error) {
    printnl("✗ Test 1 failed: ", $error);
}

// Test 2: XPath with predicates
printnl("Test 2: XPath with predicates");
try {
    Xml2 $xml2 = new Xml2();
    XmlDocument $doc2 = $xml2->createFromString($testXmlContent);
    
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
} catch (string $error) {
    printnl("✗ Test 2 failed: ", $error);
}

// Test 3: XPath result types
printnl("Test 3: XPath result types");
try {
    Xml2 $xml3 = new Xml2();
    XmlDocument $doc3 = $xml3->createFromString($testXmlContent);
    
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
} catch (string $error) {
    printnl("✗ Test 3 failed: ", $error);
}

// Test 4: XPath object creation
printnl("Test 4: XPath object creation");
try {
    Xml2 $xml4 = new Xml2();
    XmlDocument $doc4 = $xml4->createFromString($testXmlContent);
    
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
        }
    }
} catch (string $error) {
    printnl("✗ Test 4 failed: ", $error);
}

printnl("=== XPath Phase 2 Test Complete ===");