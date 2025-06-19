printnl("=== XPath Test ===");

const string $xml = "<?xml version=\"1.0\"?><books><book id=\"1\"><title>Test Book</title><author>Test Author</author></book><book id=\"2\"><title>Another Book</title><author>Another Author</author></book></books>";

Xml2 $xmlHandler = new Xml2();
XmlDocument $doc = $xmlHandler->createFromString($xml, "");

printnl("Document created, testing XPath...");

// Test selectNodes
XmlNodeList $books = $doc->selectNodes("//book");
int $count = $books->getLength();
printnl("Found books: ", $count);

// Test selectSingleNode  
XmlNode $firstBook = $doc->selectSingleNode("//book[1]");
string $id = $firstBook->getAttribute("id");
printnl("First book ID: ", $id);

// Test XPath from node context
XmlNode $title = $firstBook->selectSingleNode("title");
string $titleText = $title->getTextContent();
printnl("Title: ", $titleText);

// Test xpath method
XmlXPathResult $result = $doc->xpath("count(//book)");
int $type = $result->getType();
printnl("XPath result type: ", $type);

double $bookCount = $result->getNumberValue();
printnl("Book count: ", $bookCount);

// Test XPath with predicates
XmlNodeList $specificBook = $doc->selectNodes("//book[@id='2']");
int $specificCount = $specificBook->getLength();
printnl("Books with ID=2: ", $specificCount);

// Test boolean XPath result
XmlXPathResult $boolResult = $doc->xpath("count(//book) > 1");
bool $hasMultipleBooks = $boolResult->getBooleanValue();
printnl("Has multiple books: ", $hasMultipleBooks);

// Test string XPath result
XmlXPathResult $stringResult = $doc->xpath("//book[1]/title/text()");
string $firstTitle = $stringResult->getStringValue();
printnl("First title: ", $firstTitle);

printnl("✓ All XPath tests completed successfully!");