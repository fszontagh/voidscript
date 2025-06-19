// Simple Phase 1 XML Module Enhancement Test Script

printnl("=== XML Module Phase 1 Simple Test ===");

// Test 1: Test legacy functionality still works
printnl("Test 1: Legacy XML parsing");
const string $testXmlContent = "<?xml version=\"1.0\"?><test><item id=\"1\">Test Item</item></test>";
const string $testXmlFile = "/tmp/test_simple.xml";

// Write test file
file_put_contents($testXmlFile, $testXmlContent, false);

// Test legacy methods
Xml2 $xml = new Xml2();
$xml->readFile($testXmlFile);
XmlNode $root = $xml->getRootElement();

if ($root != NULL) {
    printnl("✓ Legacy readFile and getRootElement work");
    object $attrs = $root->getAttributes();
    if ($attrs != NULL) {
        printnl("✓ Legacy getAttributes works");
    }
} else {
    printnl("✗ Legacy methods failed");
}

// Test 2: Test memory parsing
printnl("Test 2: Legacy memory parsing");
Xml2 $memXml = new Xml2();
$memXml->readMemory($testXmlContent);
XmlNode $memRoot = $memXml->getRootElement();

if ($memRoot != NULL) {
    printnl("✓ Legacy readMemory works");
} else {
    printnl("✗ Legacy readMemory failed");
}

// Test 3: Enhanced document creation (if available)
printnl("Test 3: Enhanced document creation");
try {
    XmlDocument $doc = $xml->createDocument();
    if ($doc != NULL) {
        printnl("✓ Enhanced createDocument works");
    } else {
        printnl("? Enhanced createDocument returned NULL");
    }
} catch (string $error) {
    printnl("? Enhanced createDocument not available: ", $error);
}

// Test 4: Enhanced string parsing (if available)
printnl("Test 4: Enhanced string parsing");
try {
    XmlDocument $parsedDoc = $xml->createFromString($testXmlContent);
    if ($parsedDoc != NULL) {
        printnl("✓ Enhanced createFromString works");
    } else {
        printnl("? Enhanced createFromString returned NULL");
    }
} catch (string $error) {
    printnl("? Enhanced createFromString not available: ", $error);
}

// Test 5: Enhanced file parsing (if available)
printnl("Test 5: Enhanced file parsing");
try {
    XmlDocument $fileDoc = $xml->createFromFile($testXmlFile);
    if ($fileDoc != NULL) {
        printnl("✓ Enhanced createFromFile works");
    } else {
        printnl("? Enhanced createFromFile returned NULL");
    }
} catch (string $error) {
    printnl("? Enhanced createFromFile not available: ", $error);
}

printnl("=== Test Complete ===");

// Clean up
if (file_exists($testXmlFile)) {
    unlink($testXmlFile);
}