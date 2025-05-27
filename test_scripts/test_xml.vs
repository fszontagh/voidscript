printnl("=== TESTING XML MODULE ===");

// Test creating a simple XML document
class Xml2 $xml = new Xml2();

// Create a root element
class XmlNode $root = $xml.createElement("users");

// Create a user element with attributes
class XmlNode $user = $xml.createElement("user");
class XmlAttr $idAttr = $user.createAttribute("id", "123");
class XmlAttr $nameAttr = $user.createAttribute("name", "John Doe");

// Add text content
$user.setTextContent("Active user");

// Add user to root
$root.appendChild($user);

// Set root on document
$xml.setRootElement($root);

// Output the XML
string $xmlString = $xml.toString();
printnl("Generated XML:");
printnl($xmlString);

printnl("XML module test successful!");
