// Simple XPath Test
function main() {
    print("=== XML XPath Simple Test ===\n");
    
    var xml2 = new Xml2();
    
    var testXml = "<bookstore><book id=\"1\"><title>Test Book</title><author>Test Author</author></book></bookstore>";
    
    print("Creating XML document...\n");
    var doc = xml2.createFromString(testXml);
    print("Document created successfully\n");
    
    // Test basic XPath
    print("Testing selectNodes...\n");
    var books = doc.selectNodes("//book");
    print("Found books: ");
    print(books.getLength());
    print("\n");
    
    // Test single node selection
    print("Testing selectSingleNode...\n");
    var firstBook = doc.selectSingleNode("//book[1]");
    if (firstBook != null) {
        var title = firstBook.selectSingleNode("title");
        if (title != null) {
            print("First book title: ");
            print(title.getTextContent());
            print("\n");
        }
    }
    
    // Test xpath result
    print("Testing xpath() method...\n");
    var countResult = doc.xpath("count(//book)");
    print("XPath result type: ");
    print(countResult.getType());
    print("\n");
    
    if (countResult.getType() == 3) {
        print("Number of books: ");
        print(countResult.getNumberValue());
        print("\n");
    }
    
    print("=== XPath test completed ===\n");
    return 0;
}