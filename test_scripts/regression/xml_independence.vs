// Regression: Xml2 was fully broken and crashed at teardown. Fixed:
//  - $class_name marker (was __class__) so method dispatch works;
//  - per-object handle reads (was CLASS-level) so documents are independent;
//  - teardown SIGSEGV (inline-static holders destroyed out of order with ~XmlModule -
//    ~XmlModule no longer touches them; valgrind reports 0 errors);
//  - getRootElement bridges createFromString documents (dual storage systems);
//  - isWellFormed checked DTD validity, not well-formedness;
//  - createFromString baseUrl is now actually optional.
// Expected: clean exit 0.

Xml2 $a = new Xml2();
Xml2 $b = new Xml2();
XmlDocument $d1 = $a->createFromString("<alpha><x>1</x></alpha>");   // baseUrl optional
XmlDocument $d2 = $b->createFromString("<beta><y>2</y></beta>");

// dispatch + well-formedness
printnl($d1->isWellFormed());               // true
printnl($d2->isWellFormed());               // true

// navigation, independent per document
printnl($d1->getRootElement()->getName());  // alpha
printnl($d2->getRootElement()->getName());  // beta

// serialisation, no crosstalk
string $s1 = $d1->toString("UTF-8", false);
string $s2 = $d2->toString("UTF-8", false);
printnl(string_contains($s1, "alpha") && string_contains($s2, "beta"));   // true
printnl(string_contains($s1, "beta") || string_contains($s2, "alpha"));   // false

printnl("done");
