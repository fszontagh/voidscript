// XML XPath Phase 2 Test Script
// Tests comprehensive XPath functionality

function main() {
    print("=== XML XPath Phase 2 Test ===\n");
    
    // Create test XML document with complex structure
    var xml2 = new Xml2();
    
    var testXml = "<bookstore>" +
                  "  <book id='1' category='fiction'>" +
                  "    <title lang='en'>Great Gatsby</title>" +
                  "    <author>F. Scott Fitzgerald</author>" +
                  "    <price>12.99</price>" +
                  "  </book>" +
                  "  <book id='2' category='fiction'>" +
                  "    <title lang='en'>To Kill a Mockingbird</title>" +
                  "    <author>Harper Lee</author>" +
                  "    <price>14.99</price>" +
                  "  </book>" +
                  "  <book id='3' category='biography'>" +
                  "    <title lang='en'>Steve Jobs</title>" +
                  "    <author>Walter Isaacson</author>" +
                  "    <price>16.99</price>" +
                  "  </book>" +
                  "</bookstore>";
    
    print("Creating XML document from string...\n");
    var doc = xml2.createFromString(testXml);
    print("Document created successfully\n\n");
    
    // Test 1: Basic XPath - select all books
    print("=== Test 1: Select all books ===\n");
    var allBooks = doc.selectNodes("//book");
    print("Found " + allBooks.getLength() + " books\n");
    
    for (var i = 0; i < allBooks.getLength(); i++) {
        var book = allBooks.item(i);
        var title = book.selectSingleNode("title").getTextContent();
        var id = book.getAttribute("id");
        print("Book " + id + ": " + title + "\n");
    }
    print("\n");
    
    // Test 2: XPath with predicates - fiction books only
    print("=== Test 2: Select fiction books ===\n");
    var fictionBooks = doc.selectNodes("//book[@category='fiction']");
    print("Found " + fictionBooks.getLength() + " fiction books\n");
    
    for (var i = 0; i < fictionBooks.getLength(); i++) {
        var book = fictionBooks.item(i);
        var title = book.selectSingleNode("title").getTextContent();
        var author = book.selectSingleNode("author").getTextContent();
        print("Fiction: " + title + " by " + author + "\n");
    }
    print("\n");
    
    // Test 3: XPath functions - books with price > 15
    print("=== Test 3: Books with price > 15 ===\n");
    var expensiveBooks = doc.selectNodes("//book[price > 15]");
    print("Found " + expensiveBooks.getLength() + " expensive books\n");
    
    for (var i = 0; i < expensiveBooks.getLength(); i++) {
        var book = expensiveBooks.item(i);
        var title = book.selectSingleNode("title").getTextContent();
        var price = book.selectSingleNode("price").getTextContent();
        print("Expensive: " + title + " - $" + price + "\n");
    }
    print("\n");
    
    // Test 4: Single node selection
    print("=== Test 4: Select first book ===\n");
    var firstBook = doc.selectSingleNode("//book[1]");
    if (firstBook != null) {
        var title = firstBook.selectSingleNode("title").getTextContent();
        var author = firstBook.selectSingleNode("author").getTextContent();
        print("First book: " + title + " by " + author + "\n");
    }
    print("\n");
    
    // Test 5: XPath from node context
    print("=== Test 5: XPath from node context ===\n");
    var secondBook = doc.selectSingleNode("//book[2]");
    if (secondBook != null) {
        var title = secondBook.selectSingleNode("title").getTextContent();
        print("Second book title: " + title + "\n");
        
        // Select relative to this book
        var authorFromBook = secondBook.selectSingleNode("author").getTextContent();
        print("Author from book context: " + authorFromBook + "\n");
    }
    print("\n");
    
    // Test 6: XPath result object
    print("=== Test 6: XPath result object ===\n");
    var xpathResult = doc.xpath("count(//book)");
    print("XPath result type: " + xpathResult.getType() + "\n");
    if (xpathResult.getType() == 3) { // NUMBER type
        print("Number of books: " + xpathResult.getNumberValue() + "\n");
    }
    
    var stringResult = doc.xpath("//book[1]/title/text()");
    print("String result type: " + stringResult.getType() + "\n");
    if (stringResult.getType() == 4) { // STRING type  
        print("First book title: " + stringResult.getStringValue() + "\n");
    }
    print("\n");
    
    // Test 7: Advanced XPath - text content
    print("=== Test 7: Advanced XPath queries ===\n");
    var authors = doc.selectNodes("//author/text()");
    print("All authors (" + authors.getLength() + "):\n");
    for (var i = 0; i < authors.getLength(); i++) {
        var author = authors.item(i);
        print("- " + author.getTextContent() + "\n");
    }
    print("\n");
    
    // Test 8: XPath with attributes
    print("=== Test 8: XPath with attributes ===\n");
    var englishTitles = doc.selectNodes("//title[@lang='en']");
    print("English titles (" + englishTitles.getLength() + "):\n");
    for (var i = 0; i < englishTitles.getLength(); i++) {
        var title = englishTitles.item(i);
        print("- " + title.getTextContent() + "\n");
    }
    print("\n");
    
    print("=== All XPath tests completed successfully! ===\n");
    return 0;
}