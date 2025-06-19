// XML XPath Advanced Features Test
// Tests XPath object creation, namespaces, and variables

function main() {
    print("=== XML XPath Advanced Features Test ===\n");
    
    var xml2 = new Xml2();
    
    // Create test XML with namespaces
    var xmlWithNamespaces = "<?xml version='1.0'?>" +
                           "<books xmlns:fiction='http://example.com/fiction' xmlns:bio='http://example.com/biography'>" +
                           "  <fiction:book id='1'>" +
                           "    <fiction:title>1984</fiction:title>" +
                           "    <fiction:author>George Orwell</fiction:author>" +
                           "  </fiction:book>" +
                           "  <bio:book id='2'>" +
                           "    <bio:title>Einstein</bio:title>" +
                           "    <bio:author>Walter Isaacson</bio:author>" +
                           "  </bio:book>" +
                           "</books>";
    
    print("Creating XML document with namespaces...\n");
    var doc = xml2.createFromString(xmlWithNamespaces);
    print("Document created successfully\n\n");
    
    // Test 1: Create XPath object
    print("=== Test 1: Create XPath Object ===\n");
    var xpath = xml2.createXPath(doc);
    print("XPath object created successfully\n\n");
    
    // Test 2: Register namespaces
    print("=== Test 2: Register Namespaces ===\n");
    xpath.registerNamespace("f", "http://example.com/fiction");
    xpath.registerNamespace("b", "http://example.com/biography");
    print("Namespaces registered successfully\n\n");
    
    // Test 3: XPath with namespaces
    print("=== Test 3: XPath with Namespaces ===\n");
    var fictionResult = xpath.evaluate("//f:book");
    print("Fiction books result type: " + fictionResult.getType() + "\n");
    if (fictionResult.getType() == 1) { // NODESET
        var fictionBooks = fictionResult.getNodeSet();
        print("Found " + fictionBooks.getLength() + " fiction books\n");
        
        for (var i = 0; i < fictionBooks.getLength(); i++) {
            var book = fictionBooks.item(i);
            var id = book.getAttribute("id");
            print("Fiction book ID: " + id + "\n");
        }
    }
    print("\n");
    
    // Test 4: Register variables
    print("=== Test 4: Register Variables ===\n");
    xpath.registerVariable("minId", "1");
    xpath.registerVariable("maxPrice", 20.0);
    xpath.registerVariable("includeAll", true);
    print("Variables registered successfully\n\n");
    
    // Test 5: Boolean XPath result
    print("=== Test 5: Boolean XPath Result ===\n");
    var boolResult = xpath.evaluate("count(//f:book) > 0");
    print("Boolean result type: " + boolResult.getType() + "\n");
    if (boolResult.getType() == 2) { // BOOLEAN
        print("Has fiction books: " + boolResult.getBooleanValue() + "\n");
    }
    print("\n");
    
    // Test 6: Number XPath result
    print("=== Test 6: Number XPath Result ===\n");
    var countResult = xpath.evaluate("count(//book)");
    print("Count result type: " + countResult.getType() + "\n");
    if (countResult.getType() == 3) { // NUMBER
        print("Total books: " + countResult.getNumberValue() + "\n");
    }
    print("\n");
    
    // Test 7: String XPath result
    print("=== Test 7: String XPath Result ===\n");
    var stringResult = xpath.evaluate("//f:book/f:title/text()");
    print("String result type: " + stringResult.getType() + "\n");
    if (stringResult.getType() == 4) { // STRING
        print("First fiction title: " + stringResult.getStringValue() + "\n");
    }
    print("\n");
    
    // Test 8: Complex XPath expressions
    print("=== Test 8: Complex XPath Expressions ===\n");
    
    // Test node-set size
    var allBooksResult = xpath.evaluate("//book");
    if (allBooksResult.getType() == 1) {
        print("Total books found: " + allBooksResult.getNodeSetSize() + "\n");
    }
    
    // Test with position functions
    var firstBookResult = xpath.evaluate("//book[1]");
    if (firstBookResult.getType() == 1) {
        var firstBooks = firstBookResult.getNodeSet();
        if (firstBooks.getLength() > 0) {
            var firstBook = firstBooks.item(0);
            var id = firstBook.getAttribute("id");
            print("First book ID: " + id + "\n");
        }
    }
    print("\n");
    
    // Test 9: Document-level XPath shortcuts
    print("=== Test 9: Document XPath Shortcuts ===\n");
    var directResult = doc.xpath("count(//book)");
    print("Direct xpath() result type: " + directResult.getType() + "\n");
    if (directResult.getType() == 3) {
        print("Books count via document.xpath(): " + directResult.getNumberValue() + "\n");
    }
    
    var directNodes = doc.selectNodes("//book");
    print("Direct selectNodes() found: " + directNodes.getLength() + " books\n");
    
    var directSingle = doc.selectSingleNode("//book[1]");
    if (directSingle != null) {
        var id = directSingle.getAttribute("id");
        print("Direct selectSingleNode() first book ID: " + id + "\n");
    }
    print("\n");
    
    print("=== All advanced XPath tests completed successfully! ===\n");
    return 0;
}