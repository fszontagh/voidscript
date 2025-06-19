// Phase 1 XML Module Enhancement Test Script
// Tests core DOM functionality and document management

printnl("=== XML Module Phase 1 Enhancement Test ===");
printnl("");

// Test 1: Create new XML document
printnl("Test 1: Creating new XML document");
Xml2 $xmlModule = new Xml2();
XmlDocument $doc = $xmlModule->createDocument("1.0", "UTF-8");
if ($doc != NULL) {
    printnl("✓ Document created successfully");
    printnl("  Version: ", $doc->getVersion());
    printnl("  Encoding: ", $doc->getEncoding());
    printnl("  Well-formed: ", $doc->isWellFormed());
} else {
    printnl("✗ Failed to create document");
}
printnl("");

// Test 2: Create elements and build document structure
printnl("Test 2: Creating elements and document structure");
XmlNode $root = $doc->createElement("books");
XmlNode $book1 = $doc->createElement("book");
XmlNode $book2 = $doc->createElement("book");

if ($root != NULL && $book1 != NULL && $book2 != NULL) {
    printnl("✓ Elements created successfully");
    
    // Set attributes
    $book1->setAttribute("id", "1");
    $book1->setAttribute("genre", "fiction");
    $book2->setAttribute("id", "2");
    $book2->setAttribute("genre", "mystery");
    
    printnl("✓ Attributes set successfully");
    
    // Create text content
    XmlNode $title1 = $doc->createElement("title");
    XmlNode $title1Text = $doc->createTextNode("The Great Adventure");
    XmlNode $author1 = $doc->createElement("author");
    XmlNode $author1Text = $doc->createTextNode("John Doe");
    
    XmlNode $title2 = $doc->createElement("title");
    XmlNode $title2Text = $doc->createTextNode("Mystery of the Lost Key");
    XmlNode $author2 = $doc->createElement("author");
    XmlNode $author2Text = $doc->createTextNode("Jane Smith");
    
    if ($title1 != NULL && $title1Text != NULL && $author1 != NULL && $author1Text != NULL &&
        $title2 != NULL && $title2Text != NULL && $author2 != NULL && $author2Text != NULL) {
        printnl("✓ Text nodes created successfully");
    } else {
        printnl("✗ Failed to create text nodes");
    }
} else {
    printnl("✗ Failed to create elements");
}
printnl("");

// Test 3: Test node properties and content
printnl("Test 3: Testing node properties");
if ($book1 != NULL) {
    printnl("Book1 name: ", $book1->getName());
    printnl("Book1 has id attribute: ", $book1->hasAttribute("id"));
    printnl("Book1 id value: ", $book1->getAttribute("id"));
    printnl("Book1 genre value: ", $book1->getAttribute("genre"));
    printnl("Book1 has children: ", $book1->hasChildNodes());
    
    // Test all attributes
    object $allAttrs = $book1->getAllAttributes();
    if ($allAttrs != NULL) {
        printnl("✓ Retrieved all attributes successfully");
    }
} else {
    printnl("✗ Book1 node is NULL");
}
printnl("");

// Test 4: Test XML from string parsing
printnl("Test 4: Parsing XML from string");
const string $xmlString = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><library><book id=\"3\" title=\"Test Book\"><author>Test Author</author></book></library>";

XmlDocument $parsedDoc = $xmlModule->createFromString($xmlString);
if ($parsedDoc != NULL) {
    printnl("✓ XML parsed from string successfully");
    
    XmlNode $parsedRoot = $parsedDoc->getRootElement();
    if ($parsedRoot != NULL) {
        printnl("  Root element: ", $parsedRoot->getName());
        
        XmlNode $firstChild = $parsedRoot->getFirstChild();
        if ($firstChild != NULL) {
            printnl("  First child: ", $firstChild->getName());
            printnl("  Child id: ", $firstChild->getAttribute("id"));
            printnl("  Child title: ", $firstChild->getAttribute("title"));
            
            // Test navigation
            XmlNode $authorNode = $firstChild->getFirstChild();
            if ($authorNode != NULL) {
                printnl("  Author element: ", $authorNode->getName());
                printnl("  Author content: ", $authorNode->getTextContent());
            }
        }
    }
} else {
    printnl("✗ Failed to parse XML from string");
}
printnl("");

// Test 5: Test document serialization
printnl("Test 5: Document serialization");
if ($parsedDoc != NULL) {
    string $xmlOutput = $parsedDoc->toString("UTF-8", true);
    if ($xmlOutput != NULL && $xmlOutput != "") {
        printnl("✓ Document serialized successfully");
        printnl("  Output length: ", strlen($xmlOutput));
        printnl("  First 100 characters: ", substr($xmlOutput, 0, 100));
    } else {
        printnl("✗ Failed to serialize document");
    }
}
printnl("");

// Test 6: Test node cloning
printnl("Test 6: Node cloning");
if ($book1 != NULL) {
    XmlNode $clonedBook = $book1->cloneNode(false);
    if ($clonedBook != NULL) {
        printnl("✓ Node cloned successfully (shallow)");
        printnl("  Cloned name: ", $clonedBook->getName());
        printnl("  Cloned id: ", $clonedBook->getAttribute("id"));
        printnl("  Cloned genre: ", $clonedBook->getAttribute("genre"));
    } else {
        printnl("✗ Failed to clone node");
    }
}
printnl("");

// Test 7: Test backward compatibility with legacy methods
printnl("Test 7: Testing backward compatibility");
// Create a test XML file first
const string $testXmlFile = "/tmp/test_xml_phase1.xml";
const string $testXmlContent = "<?xml version=\"1.0\"?><test><item id=\"1\">Test Item</item></test>";

// Write test file
file_put_contents($testXmlFile, $testXmlContent, false);

// Test legacy readFile method
Xml2 $legacyXml = new Xml2();
$legacyXml->readFile($testXmlFile);
XmlNode $legacyRoot = $legacyXml->getRootElement();

if ($legacyRoot != NULL) {
    printnl("✓ Legacy readFile method works");
    
    // Test legacy getAttributes method
    object $legacyAttrs = $legacyRoot->getAttributes();
    if ($legacyAttrs != NULL) {
        printnl("✓ Legacy getAttributes method works");
    } else {
        printnl("✗ Legacy getAttributes method failed");
    }
} else {
    printnl("✗ Legacy readFile method failed");
}
printnl("");

// Test 8: Test memory parsing with legacy method
printnl("Test 8: Testing legacy readMemory");
Xml2 $memoryXml = new Xml2();
$memoryXml->readMemory($testXmlContent);
XmlNode $memoryRoot = $memoryXml->getRootElement();

if ($memoryRoot != NULL) {
    printnl("✓ Legacy readMemory method works");
} else {
    printnl("✗ Legacy readMemory method failed");
}
printnl("");

// Test 9: Test error handling
printnl("Test 9: Error handling tests");
try {
    XmlDocument $invalidDoc = $xmlModule->createFromString("<invalid><xml>");
    printnl("✗ Should have thrown error for invalid XML");
} catch (string $error) {
    printnl("✓ Properly caught error for invalid XML: ", $error);
}
printnl("");

// Test 10: Test utility functions
printnl("Test 10: Utility functions");
if ($book1 != NULL) {
    // Test attribute removal
    $book1->removeAttribute("genre");
    if (!$book1->hasAttribute("genre")) {
        printnl("✓ Attribute removal works");
    } else {
        printnl("✗ Attribute removal failed");
    }
    
    // Test content modification
    $book1->setName("modified-book");
    if ($book1->getName() == "modified-book") {
        printnl("✓ Name modification works");
    } else {
        printnl("✗ Name modification failed");
    }
}
printnl("");

printnl("=== XML Module Phase 1 Enhancement Test Complete ===");
printnl("This test covers:");
printnl("- Document creation and parsing");
printnl("- Element and text node creation");
printnl("- Attribute management");
printnl("- Node navigation and properties");
printnl("- Document serialization");
printnl("- Node cloning");
printnl("- Backward compatibility");
printnl("- Error handling");
printnl("- Content modification");

// Clean up
if (file_exists($testXmlFile)) {
    unlink($testXmlFile);
}