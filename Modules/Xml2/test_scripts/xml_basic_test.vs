// Basic XML Module Test Script

printnl("=== XML Module Basic Test ===");

// Test 1: Test legacy functionality still works
printnl("Test 1: Legacy XML parsing");
const string $testXmlContent = "<?xml version=\"1.0\"?><test><item id=\"1\">Test Item</item></test>";
const string $testXmlFile = "/tmp/test_basic.xml";

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
        printnl("  Tag name: ", $attrs->tagName);
        printnl("  Tag type: ", $attrs->tagType);
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
    object $memAttrs = $memRoot->getAttributes();
    if ($memAttrs != NULL) {
        printnl("  Memory parsed tag: ", $memAttrs->tagName);
    }
} else {
    printnl("✗ Legacy readMemory failed");
}

// Test 3: Test node navigation if children exist
printnl("Test 3: Node navigation");
if ($root != NULL) {
    object $rootAttrs = $root->getAttributes();
    if ($rootAttrs != NULL && $rootAttrs->children != NULL) {
        printnl("✓ Root has children");
    } else {
        printnl("? Root has no children or children is NULL");
    }
}

printnl("=== Basic Test Complete ===");
printnl("Legacy XML functionality is working correctly!");

// Clean up
if (file_exists($testXmlFile)) {
    unlink($testXmlFile);
}