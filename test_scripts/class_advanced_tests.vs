<?void
printnl("--- Constructor Tests ---");
class NoArgConstructor {
    string $message = "Default";
    // No explicit 'construct' method
    function getMessage() string { return $this->message; }
}
NoArgConstructor $nac = new NoArgConstructor();
printnl("NoArgConstructor message:", $nac->getMessage()); // Expected: Default

class ConstructorWithArgs {
    string $name;
    int $value;
    function construct(string $n, int $v) {
        $this->name = $n;
        $this->value = $v;
        printnl("ConstructorWithArgs called:", $this->name, $this->value);
    }
    function getData() string { return $this->name + ":" + $this->value; }
}
ConstructorWithArgs $cwa = new ConstructorWithArgs("Test", 123);
printnl("ConstructorWithArgs data:", $cwa->getData()); // Expected: Test:123

// Test for constructor argument count mismatch (should ideally throw error)
// The interpreter should throw an error here.
// How to test for caught errors in script? (VoidScript does not have try-catch)
// For now, document that this should cause an error.
// printnl("Testing arg mismatch (expected error):");
// // new ConstructorWithArgs("TooFew"); // This line should cause an error.

// Test for constructor with default property values and constructor modifying them
class DefaultAndConstruct {
    string $status = "Initial";
    function construct(string $s) {
        $this->status = $s;
        printnl("DefaultAndConstruct status set to:", $this->status);
    }
    function getStatus() string { return $this->status; }
}
DefaultAndConstruct $dac = new DefaultAndConstruct("Constructed");
printnl("DefaultAndConstruct status:", $dac->getStatus()); // Expected: Constructed

printnl(""); // Newline for separation
printnl("--- 'this' and Property Modification Tests ---");
class PropertyTester {
    int $counter = 0;
    string $name = "OriginalName";

    function increment() {
        $this->counter = $this->counter + 1;
    }
    function getCounter() int { return $this->counter; }

    function setName(string $newName) {
        printnl("setName: current name =", $this->name);
        $this->name = $newName;
        printnl("setName: new name =", $this->name);
    }
    function getName() string { return $this->name; }

    function callAnotherMethod() {
        $this->setName("ChangedViaInternalCall");
    }
}

PropertyTester $pt = new PropertyTester();
$pt->increment();
$pt->increment();
printnl("Counter after 2 increments:", $pt->getCounter()); // Expected: 2

$pt->setName("FirstChange");
printnl("Name after setName:", $pt->getName()); // Expected: FirstChange

$pt->callAnotherMethod();
printnl("Name after callAnotherMethod:", $pt->getName()); // Expected: ChangedViaInternalCall

// Test direct property modification if language supports it (e.g. $pt->$counter = 10;)
// Assuming direct public property access and modification is allowed:
// If direct property access isn't a feature or is handled differently, this part should be skipped or adapted.
// For VoidScript, direct public property access like $object->$property is used for reading.
// Assignment $object->$property = value; is not standard for method-based languages unless specifically implemented.
// The AssignmentStatementNode handles $var = value and $obj->prop = value through specific parsing.
// $obj->$prop = value would require $prop to be a variable that holds the name of the property,
// or specific parser support for direct field assignment.
// Given the current AssignmentStatementNode, direct assignment like $pt->$counter = 10 is not supported.
// Properties are typically modified via methods (like setName).
// printnl("Attempting direct property assignment (feature may not be supported):");
// $pt->$counter = 10; 
// printnl("Counter after direct assignment:", $pt->getCounter()); 

printnl(""); // Newline for separation
printnl("--- Nested Objects and 'this' ---");
class Inner {
    string $data = "InnerData";
    function setData(string $d) { $this->data = $d; }
    function getData() string { return $this->data; }
}
class Outer {
    Inner $myInner; // Property to hold an Inner object
    string $outerName = "OuterName";

    function construct() {
        $this->myInner = new Inner(); // Instantiate Inner object and assign to property
        printnl("Outer construct: Inner object created.");
    }
    function setInnerData(string $val) {
        $this->myInner->setData($val); // Call method on inner object property
    }
    function getInnerData() string {
        return $this->myInner->getData();
    }
    function getMyInner() Inner { // Returning the object itself
         return $this->myInner;
    }
}
Outer $o = new Outer();
$o->setInnerData("UpdatedInnerValue");
printnl("Outer getting InnerData:", $o->getInnerData()); // Expected: UpdatedInnerValue

Inner $refToInner = $o->getMyInner();
$refToInner->setData("ChangedDirectlyOnRef");
printnl("Outer getting InnerData after change via ref:", $o->getInnerData()); // Expected: ChangedDirectlyOnRef
printnl("RefToInner getData:", $refToInner->getData()); // Expected: ChangedDirectlyOnRef
?>
