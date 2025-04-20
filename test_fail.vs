// smple function

function girafe (int $param1) {
  int $var = $param1;
}

object $Batman =  {
  string name: "batman"
};
// Define a simple class with defaults and two public properties
class Foo {
  public: int $a = 1;
  public: int $b = 2;
  private: string $name;

  function test() {
        printnl("Test method called");
  }
  function incrementA() int {
    $this->a = $this->a + 1;
    return this->a;
  }

  function SetName(string $newName) null {
    $this->name = $newName;
  }
}

// Instantiate with one constructor arg (overrides $a, leaves $b at default)
Foo $f = new Foo(); // there is no constructor, so no parameters

// Should print “10”
printnl("$f->a = ", $f->a);
printnl("$f->b = ", $f->b);

// Should print “2”
printnl("$f->b + $f->a = ",$f->b + $f->a);

// typeof returns a string “object”
printnl("Typeof $f: ",typeof($f));

$f->test();

printnl("$f->a now = ", $f->a);

